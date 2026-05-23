#include "PlaylabsGL.h"
#include "Src/UI.h"
#include <GL/gl.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <cstdint>
#include <inttypes.h>

#include "Constants.h"
#include "DrawHelpers.h"
#include "Bullet.h"
#include "Player.h"
#ifdef _DEBUG
#include "DebugPanel.h"
#endif
// Build 0.0.19

int main()
{
    Window window;
    if (!window.create("RubberBandBattle-Shell", 1280, 720))
        return -1;
#ifdef _DEBUG
    // ---- Debug panel ----
    if (!DebugPanel_Init(GetModuleHandle(nullptr)))
        MessageBoxA(nullptr, "DebugPanel failed to initialise.", "Warning", MB_OK | MB_ICONWARNING);
#endif
    // ---- Font ----
    UI::_font::load("Resources/Font/Confale.ttf");

    Camera main_cam;
    main_cam.position = {0.0f, 0.0f};
    main_cam.zoom     = 1.723f;

    Timer timer;
    float fpsTimer = 0.0f, currentFPS = 0.0f;
    int   fpsFrames = 0;
    float promptPulse = 0.0f;

    Sound* bgm      = CreateSound(); bgm->load("bgm.wav");      bgm->play(true);
    Sound* sfxJump  = CreateSound(); sfxJump->load("jump.wav");
    Sound* sfxPunch = CreateSound(); sfxPunch->load("punch.wav");

    // ---- Assets ----
    Image* playersheet = PL_LoadImage("Resources/Skins/spritemap.png");
    Atlas* playeratlas = LoadAtlas("Resources/Skins/spritemap.json");

    // ---- Player ----
    Player player;
    player.sprite                 = CreateSprite();
    player.sprite->image          = playersheet;
    player.sprite->atlas          = playeratlas;
    player.sprite->frameName      = "0001";
    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {1.0f, 1.0f};
    player.sprite->targetScale    = {1.0f, 1.0f};

    player.anim = CreateAnimator();
    if (player.anim->load("Resources/Skins/Animation.json"))
        Anim(player.anim, PLAYER, IDLE);

    // ---- Background ----
    Sprite* background         = CreateSprite();
    background->image          = playersheet;
    background->atlas          = playeratlas;
    background->frameName      = "0001";
    background->position       = {0.0f, 0.0f};
    background->targetPosition = {0.0f, 0.0f};

    std::vector<Bullet> bullets;
    bullets.reserve(64);

    bool prevMouseHeld  = false;
    bool prevRMouseHeld = false;

    const float wX = 100.0f, wY = 100.0f, wW = 100.0f, wH = 100.0f;

    // Animation state
    Player::State lastAnimState = Player::State::IDLE;

    // Death / respawn
    bool  isDead           = false;
    float respawnTimer     = 0.0f;
    bool  deathAnimDone    = false;
    float deathAnimTimer   = 0.0f;
    const float RESPAWN_DELAY  = 3.0f;
    const float DEATH_ANIM_DUR = 1.0f;

    // Spawn position — mutable so debug panel sliders can change them
    float SPAWN_X = 200.0f;
    float SPAWN_Y = 436.0f;

    // ==========================================================
    // GAME LOOP
    // ==========================================================
    while (window.process())
    {
        float dt = timer.delta();
        if (dt > 0.05f) dt = 0.05f;
        Sleep(10);

        fpsTimer += dt; ++fpsFrames;
        if (fpsTimer >= 1.0f)
        {
            currentFPS = fpsFrames / fpsTimer;
            fpsFrames  = 0; fpsTimer = 0.0f;
        }

        PollInput(&window);
        int sw = window.getWidth(), sh = window.getHeight();
        promptPulse += dt;
#ifdef _DEBUG
        // ---- Debug panel tick + command dispatch ----
        DebugPanel_Tick();
        DebugPanel_PollCommands(
            isDead ? nullptr : &player,
            SPAWN_X, SPAWN_Y,
            isDead,
            (int)MAX_AMMO
        );
        DebugPanel_UpdateStatus(
            (int)player.hp, (int)player.maxHp,
            (int)player.ammo, (int)MAX_AMMO
        );
#endif
        // ---- Death check ----
        if (player.hp <= 0 && !isDead)
        {
            isDead         = true;
            respawnTimer   = RESPAWN_DELAY;
            deathAnimDone  = false;
            deathAnimTimer = 0.0f;
            Anim(player.anim, PLAYER, DIE);
            lastAnimState  = Player::State::IDLE;
        }

        // ---- Respawn countdown ----
        if (isDead)
        {
            respawnTimer -= dt;
            if (respawnTimer <= 0.0f)
            {
                isDead       = false;
                player.hp    = player.maxHp;
                player.x     = SPAWN_X;
                player.y     = SPAWN_Y;
                player.baseY = SPAWN_Y;
                player.velocityY = 0.0f;
                player.jumping   = false;
                player.punching  = false;
                player.punchCooldown = 0.0f;
                player.shootCooldown = 0.0f;
                player.ammo  = MAX_AMMO;
                player.sprite->position       = {player.x, player.y};
                player.sprite->targetPosition = {player.x, player.y};
                deathAnimDone  = false;
                deathAnimTimer = 0.0f;
                Anim(player.anim, PLAYER, IDLE);
                lastAnimState = Player::State::IDLE;
            }
        }

        // ======================================================
        // GAME LOGIC (only if alive)
        // ======================================================
        if (!isDead)
        {
            // ---- Mouse / aim ----
            int mousePixelX = 0, mousePixelY = 0;
            MousePos(&mousePixelX, &mousePixelY);

            float mouseWorldX, mouseWorldY;
            screenToWorld((float)mousePixelX, (float)mousePixelY,
                          main_cam, sw, sh, mouseWorldX, mouseWorldY);

            float pullDX  = mouseWorldX - player.muzzleX();
            float pullDY  = mouseWorldY - player.muzzleY();
            float pullLen = sqrtf(pullDX * pullDX + pullDY * pullDY);

            if (pullLen > 0.001f)
            {
                player.aimDirX = -pullDX / pullLen;
                player.aimDirY = -pullDY / pullLen;
            }

            player.facingX = (pullDX >= 0.0f) ? 1.0f : -1.0f;

            player.shootCooldown -= dt;
            player.punchCooldown -= dt;

            // ---- Left-click: shoot ----
            bool mouseHeld   = KeyDown(VK_LBUTTON) != 0;
            bool justClicked = mouseHeld && !prevMouseHeld;
            prevMouseHeld    = mouseHeld;

            if (player.ammo > 0 && justClicked && player.shootCooldown <= 0.0f)
            {
                Bullet b;
                b.x    = player.muzzleX() - 24.0f;
                b.y    = player.muzzleY() - 24.0f;
                b.velX = player.aimDirX * pullLen;
                b.velY = player.aimDirY * pullLen;
                b.sprite                 = CreateSprite();
                b.sprite->image          = playersheet;
                b.sprite->atlas          = playeratlas;
                b.sprite->frameName      = "0000";
                b.sprite->scale          = {player.facingX, 1.0f};
                b.sprite->targetScale    = {player.facingX, 1.0f};
                b.sprite->position       = {b.x, b.y};
                b.sprite->targetPosition = {b.x, b.y};
                bullets.push_back(b);
                --player.ammo;
                player.shootCooldown = SHOOT_COOLDOWN;
                Event("bullet_fired", &player, &bullets.back());
            }

            // ---- Right-click: punch ----
            bool rMouseHeld   = KeyDown(VK_RBUTTON) != 0;
            bool rJustPressed = rMouseHeld && !prevRMouseHeld;
            prevRMouseHeld    = rMouseHeld;

            if (rJustPressed && !player.punching && player.punchCooldown <= 0.0f && !player.jumping)
            {
                player.punching   = true;
                player.punchTimer = 0.0f;
                player.punchHit   = false;
                Anim(player.anim, PLAYER, PUNCH);
                lastAnimState = Player::State::PUNCH;
                sfxPunch->play(false);
                Event("punch_started", &player);
            }

            // ---- WASD: movement ----
            bool  moving = false;
            float moveX  = 0.0f, moveY = 0.0f;

            if (KeyDown(VK_RIGHT) || KeyDown('D')) { moveX =  1.0f; moving = true; }
            if (KeyDown(VK_LEFT)  || KeyDown('A')) { moveX = -1.0f; moving = true; }

            if (!player.jumping && !player.punching)
            {
                if (KeyDown(VK_UP)   || KeyDown('W')) { moveY = -1.0f; moving = true; }
                if (KeyDown(VK_DOWN) || KeyDown('S')) { moveY =  1.0f; moving = true; }
            }

            // ---- Space: jump ----
            if (KeyPressed(VK_SPACE) && !player.jumping && !player.punching)
            {
                player.baseY     = player.y;
                player.velocityY = player.jumpForce;
                player.jumping   = true;
                Anim(player.anim, PLAYER, JUMP);
                lastAnimState = Player::State::JUMP;
                sfxJump->play(false);
                Event("player_jumped", &player);
            }

            // ---- Punch tick ----
            if (player.punching)
            {
                player.punchTimer += dt;

                if (!player.punchHit)
                {
                    float phx, phy, phw, phh;
                    player.getPunchHitbox(phx, phy, phw, phh);
                    if (AABBIntersects(phx, phy, phw, phh, wX, wY, wW, wH))
                    {
                        player.punchHit = true;
                        Event("punch_hit", &player);
                    }
                }

                if (player.punchTimer >= PUNCH_DURATION)
                {
                    player.punching      = false;
                    player.punchCooldown = PUNCH_COOLDOWN;
                }
            }

            // ---- Physics ----
            if (!player.punching)
                player.x += moveX * player.speed * dt;

            if (!player.jumping)
            {
                if (!player.punching)
                {
                    player.y += moveY * player.speed * dt;
                    player.y  = std::max(player.minY, std::min(player.y, player.maxY - player.h));
                }
                player.baseY = player.y;
            }
            else
            {
                player.velocityY += player.gravity * dt;
                player.y         += player.velocityY * dt;
                if (player.y >= player.baseY)
                {
                    player.y         = player.baseY;
                    player.velocityY = 0.0f;
                    player.jumping   = false;

                    if (!player.punching)
                    {
                        if (moving && lastAnimState != Player::State::RUN)
                        {
                            Anim(player.anim, PLAYER, RUN);
                            lastAnimState = Player::State::RUN;
                        }
                        else if (!moving && lastAnimState != Player::State::IDLE)
                        {
                            Anim(player.anim, PLAYER, IDLE);
                            lastAnimState = Player::State::IDLE;
                        }
                    }

                    Event("player_landed", &player);
                }
            }

            // ---- Sprite transforms ----
            player.sprite->position       = {player.x, player.y};
            player.sprite->targetPosition = {player.x, player.y};
            player.sprite->scale          = {player.facingX, 1.0f};
            player.sprite->targetScale    = {player.facingX, 1.0f};

            SetAnimatorParent(player.anim, player.x, player.y, 0.0f, player.facingX, 1.0f);

            // ---- Animation priority block ----
            if (!player.jumping && !player.punching)
            {
                if (moving && lastAnimState != Player::State::RUN)
                {
                    Anim(player.anim, PLAYER, RUN);
                    lastAnimState = Player::State::RUN;
                }
                else if (!moving && lastAnimState != Player::State::IDLE)
                {
                    Anim(player.anim, PLAYER, IDLE);
                    lastAnimState = Player::State::IDLE;
                }
            }

            // ---- Camera follow ----
            main_cam.position.x += (player.x - (float)sw * 0.5f + 340.0f - main_cam.position.x) * 5.0f * dt;
            main_cam.position.y += (player.y - (float)sh * 0.5f + 280.0f - main_cam.position.y) * 5.0f * dt;

            // ---- Bullet update ----
            for (Bullet& b : bullets)
            {
                if (b.dead) continue;
                b.velY += BULLET_GRAVITY * dt;
                b.x    += b.velX * dt;
                b.y    += b.velY * dt;
                b.sprite->position       = {b.x, b.y};
                b.sprite->targetPosition = {b.x, b.y};
                if (b.sprite) b.sprite->update(dt);
                b.life -= dt;
                if (b.life <= 0.0f) { b.dead = true; continue; }

                if (AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH, wX, wY, wW, wH))
                {
                    b.dead = true;
                    Event("bullet_hit_wall", nullptr, &b);
                    continue;
                }

                float phbX = player.x + (player.w - 100.0f) * 0.5f;
                float phbY = player.y + player.h;
                if (AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH, phbX, phbY, 100.0f, 200.0f))
                {
                    b.dead     = true;
                    player.hp -= BULLET_DAMAGE;
                    if (player.hp < 0) player.hp = 0;
                    Event("bullet_hit_player", nullptr, &b);
                }
            }

            // Clean dead bullets
            for (Bullet& b : bullets)
                if (b.dead && b.sprite) { DestroySprite(b.sprite); b.sprite = nullptr; }
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                         [](const Bullet& b){ return b.dead; }), bullets.end());

            // ---- Wall collision ----
            float phbX = player.x + (player.w - 100.0f) * 0.5f, phbY = player.y + player.h;
            float phbW = player.jumping ? 60.0f : 100.0f, phbH = player.jumping ? 100.0f : 200.0f;
            if (AABBIntersects(phbX, phbY, phbW, phbH, wX, wY, wW, wH))
            {
                player.x -= moveX * player.speed * dt;
                player.sprite->position       = {player.x, player.y};
                player.sprite->targetPosition = {player.x, player.y};
            }

        } // end !isDead

        // ======================================================
        // RENDER — world
        // ======================================================
        PL_Clear(0.12f, 0.12f, 0.18f, 1.0f);
        applyCamera2D(main_cam, sw, sh);

        drawRect(0, player.maxY, 2000, 64, 0.25f, 0.20f, 0.15f);

        background->update(dt); background->draw(main_cam);

        for (Bullet& b : bullets) if (b.sprite) b.sprite->draw(main_cam);

        if (!isDead)
        {
            player.sprite->update(dt); player.sprite->draw(main_cam);
            TickAnimator(player.anim, dt, playersheet, playeratlas, &main_cam);
        }
        else
        {
            // Tick death anim once, then freeze on the last frame
            SetAnimatorParent(player.anim, player.x, player.y, 0.0f, player.facingX, 1.0f);
            if (!deathAnimDone)
            {
                deathAnimTimer += dt;
                TickAnimator(player.anim, dt, playersheet, playeratlas, &main_cam);
                if (deathAnimTimer >= DEATH_ANIM_DUR)
                    deathAnimDone = true;
            }
            else
            {
                TickAnimator(player.anim, 0.0f, playersheet, playeratlas, &main_cam);
            }
        }

        // Debug hitboxes (alive only)
        if (!isDead)
        {
            float dbgX = player.x + (player.w - 100.0f) * 0.5f, dbgY = player.y + player.h;
            float dbgW = player.jumping ? 60.0f : 100.0f, dbgH = player.jumping ? 100.0f : 200.0f;
            drawRect(wX,   wY,   wW,   wH,   1.0f, 0.0f, 0.0f, 0.5f);
            drawRect(dbgX, dbgY, dbgW, dbgH, 0.0f, 1.0f, 0.5f, 0.4f);
        }

        if (player.punching && !isDead)
        {
            float px, py, pw, ph;
            player.getPunchHitbox(px, py, pw, ph);
            drawRect(px, py, pw, ph,
                     player.punchHit ? 1.0f : 0.3f,
                     player.punchHit ? 0.2f : 0.6f,
                     player.punchHit ? 0.2f : 0.0f,
                     player.punchHit ? 0.9f : 0.55f);
        }

        // Spawn point crosshair (always visible in world space)
        {
            const float cs = 12.0f;  // crosshair arm length
            drawLine(SPAWN_X - cs, SPAWN_Y, SPAWN_X + cs, SPAWN_Y, 0.2f, 1.0f, 0.4f, 0.8f, 1.5f);
            drawLine(SPAWN_X, SPAWN_Y - cs, SPAWN_X, SPAWN_Y + cs, 0.2f, 1.0f, 0.4f, 0.8f, 1.5f);
        }

        // Arc preview (alive + ammo only)
        if (!isDead && player.ammo > 0)
        {
            int mx, my;
            MousePos(&mx, &my);
            float mwx, mwy;
            screenToWorld((float)mx, (float)my, main_cam, sw, sh, mwx, mwy);
            float pdx  = mwx - player.muzzleX();
            float pdy  = mwy - player.muzzleY();
            float plen = sqrtf(pdx*pdx + pdy*pdy);
            float t    = std::min(plen / MAX_POWER, 1.0f);
            float pr   = 0.2f + t*0.8f, pg = 0.8f - t*0.6f, pb = 1.0f - t;
            drawLine(player.muzzleX(), player.muzzleY(), mwx, mwy, 0.9f, 0.7f, 0.2f, 0.6f, 1.5f);
            drawArcPreview(player.muzzleX(), player.muzzleY(),
                           player.aimDirX * plen, player.aimDirY * plen, pr, pg, pb);
        }

        // ======================================================
        // RENDER — HUD
        // ======================================================
        applyScreenSpace(sw, sh);
        UI::BeginFrame(sw, sh);

        // FPS counter
        {
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", currentFPS);
            UI::Label(fpsText, (float)sw - UI::_font::textWidth(fpsText, 2.0f) - 14.0f, 18.0f,
                      2.0f, 0.3f, 1.0f, 0.3f, 1.0f);
        }

        // HP bar
        {
            float fill = std::max(0.0f, std::min((float)player.hp / (float)player.maxHp, 1.0f));
            float hr   = (fill > 0.5f) ? (1.0f - fill) * 2.0f : 1.0f;
            float hg   = (fill > 0.5f) ? 1.0f : fill * 2.0f;

            const float barW = 220.0f, barH = 18.0f;
            const float bx   = 20.0f,  by   = (float)sh - 56.0f;

            UI::Label("HP", bx, by, 2.0f, 0.4f, 1.0f, 0.4f, 1.0f);
            UI::ProgressBar(fill, bx, by + 16.0f, barW, barH,
                            hr, hg, 0.1f, 0.15f, 0.15f, 0.15f);

            char hpBuf[24];
            snprintf(hpBuf, sizeof(hpBuf), "%d / %d", (int)player.hp, (int)player.maxHp);
            UI::Label(hpBuf, bx + barW + 8.0f, by + 18.0f, 2.0f, 1, 1, 1, 0.9f);
        }

        // Respawn countdown
        if (isDead)
        {
            char respawnBuf[32];
            int  secsLeft = (int)ceilf(respawnTimer);
            snprintf(respawnBuf, sizeof(respawnBuf), "RESPAWNING IN %d...", secsLeft);
            float rw = UI::_font::textWidth(respawnBuf, 3.0f);
            UI::Label(respawnBuf, (sw - rw) * 0.5f, sh * 0.5f, 3.0f, 1.0f, 0.6f, 0.1f, 1.0f);
        }

        // Ammo / power / punch UI (alive only)
        if (!isDead)
        {
            bool outOfAmmo = (player.ammo <= 0);
            char countBuf[32];
            snprintf(countBuf, sizeof(countBuf), "AMMO  %d / %d", (int)player.ammo, (int)MAX_AMMO);
            float labelW = UI::_font::textWidth(countBuf, 2.0f);
            float labelX = (sw - labelW) * 0.5f;
            float labelY = (float)sh - 34.0f;
            UI::Label(countBuf, labelX, labelY, 2.0f,
                      1.0f, outOfAmmo ? 0.2f : 0.9f, 0.2f, 1.0f);

            if (outOfAmmo)
            {
                float pulse = fabsf(sinf(promptPulse * 3.0f));
                const char* hint = "[ RIGHT CLICK ] PUNCH";
                float hintX = (sw - UI::_font::textWidth(hint, 2.0f)) * 0.5f;
                float hintY = labelY - UI::_font::textHeight(2.0f) - 16.0f;
                UI::Label(hint, hintX, hintY, 2.0f, 1.0f, 0.3f + pulse * 0.4f, 0.1f, 1.0f);
            }

            if (player.ammo > 0)
            {
                int mx, my;
                MousePos(&mx, &my);
                float mwx, mwy;
                screenToWorld((float)mx, (float)my, main_cam, sw, sh, mwx, mwy);
                float pdx  = mwx - player.muzzleX();
                float pdy  = mwy - player.muzzleY();
                float plen = sqrtf(pdx*pdx + pdy*pdy);
                float t    = std::min(plen / MAX_POWER, 1.0f);
                float pr   = 0.2f + t*0.8f, pg = 0.8f - t*0.6f, pb = 1.0f - t;
                const float barW = 200.0f, barH = 16.0f;
                float barX = (sw - barW) * 0.5f, barY = (float)sh - 62.0f;
                char powerLabel[24];
                snprintf(powerLabel, sizeof(powerLabel), "POWER  %d", (int)plen);
                UI::Label(powerLabel, barX, barY - UI::_font::textHeight(2.0f) - 2.0f,
                          2.0f, pr, pg, pb, 1.0f);
                UI::ProgressBar(t, barX, barY, barW, barH, pr, pg, pb, 0.15f, 0.15f, 0.15f);
            }

            if (player.punchCooldown > 0.0f)
            {
                float fill = 1.0f - (player.punchCooldown / PUNCH_COOLDOWN);
                const float barW = 120.0f, barH = 12.0f;
                float barX = (sw - barW) * 0.5f, barY = (float)sh - 92.0f;
                UI::Label("PUNCH", barX, barY - UI::_font::textHeight(2.0f) - 1.0f,
                          2.0f, 1.0f, 0.5f, 0.1f, 1.0f);
                UI::ProgressBar(fill, barX, barY, barW, barH, 1.0f, 0.4f, 0.0f, 0.15f, 0.15f, 0.15f);
            }
        }

        UI::EndFrame();
        PL_Present(&window);
    }

    // ---- Cleanup ----
    for (Bullet& b : bullets) if (b.sprite) DestroySprite(b.sprite);
    EventClear();
    DestroyAnimator(player.anim); DestroySprite(player.sprite);
    DestroySprite(background);
    DestroySound(sfxPunch); DestroySound(sfxJump); DestroySound(bgm);
    FreeAtlas(playeratlas);       PL_FreeImage(playersheet);
    #ifdef _DEBUG
    DebugPanel_Destroy();
#endif
    return 0;
}
