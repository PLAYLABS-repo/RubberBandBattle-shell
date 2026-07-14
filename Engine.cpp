// Engine.cpp
// Build 0.0.19 -> refactor

#include "Engine.h"

#include <GL/gl.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <cstdint>
#include <inttypes.h>

#include "DrawHelpers.h"
#ifdef _DEBUG
#include "DebugPanel.h"
#endif

// ============================================================
// Lifecycle
// ============================================================

bool Engine::Init(const char* imagePath, const char* atlasPath)
{
    if (!window.create("RubberBandBattle-Shell", 1280, 720))
        return false;

#ifdef _DEBUG
    if (!DebugPanel_Init(GetModuleHandle(nullptr)))
        MessageBoxA(nullptr, "DebugPanel failed to initialise.", "Warning", MB_OK | MB_ICONWARNING);
#endif

    UI::_font::load("Resources/Font/Confale.ttf");

    main_cam.position = {0.0f, 0.0f};
    main_cam.zoom     = 1.723f;

    bgm      = CreateSound(); bgm->load("bgm.wav");      bgm->play(true);
    sfxJump  = CreateSound(); sfxJump->load("jump.wav");
    sfxPunch = CreateSound(); sfxPunch->load("punch.wav");

    // Sprites need to exist before the first LoadPlayerAtlas() call, since it
    // assigns image/atlas onto them rather than creating them itself.
    player.sprite                 = CreateSprite();
    player.sprite->frameName      = "0001";
    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {1.0f, 1.0f};
    player.sprite->targetScale    = {1.0f, 1.0f};

    background                 = CreateSprite();
    background->frameName      = "0001";
    background->position       = {0.0f, 0.0f};
    background->targetPosition = {0.0f, 0.0f};

    if ((player.anim = CreateAnimator())->load("Resources/Skins/Animation.json"))
        Anim(player.anim, PLAYER, IDLE);

    // Player skin (spritesheet + atlas) is loaded externally rather than hardcoded,
    // so it can point at any asset pair the caller wants to start with.
    if (!LoadPlayerAtlas(imagePath, atlasPath))
        return false;

    bullets.reserve(64);

    return true;
}

bool Engine::LoadPlayerAtlas(const char* imagePath, const char* atlasPath)
{
    Image* newImage = PL_LoadImage(imagePath);
    Atlas* newAtlas = LoadAtlas(atlasPath);

    if (!newImage || !newAtlas)
    {
        if (newImage) PL_FreeImage(newImage);
        if (newAtlas) FreeAtlas(newAtlas);
        return false;
    }

    Image* oldImage = playersheet;
    Atlas* oldAtlas = playeratlas;

    playersheet = newImage;
    playeratlas = newAtlas;

    if (player.sprite)
    {
        player.sprite->image = playersheet;
        player.sprite->atlas = playeratlas;
    }
    if (background)
    {
        background->image = playersheet;
        background->atlas = playeratlas;
    }

    // Any in-flight bullets keep using whichever sheet/atlas they were created with
    // (their sprites hold their own pointers), so no need to touch `bullets` here.

    if (oldImage) PL_FreeImage(oldImage);
    if (oldAtlas) FreeAtlas(oldAtlas);

    return true;
}

void Engine::RegisterSkin(const char* name, const char* imagePath, const char* atlasPath)
{
    skins.push_back(SkinDef{ name ? name : "", imagePath ? imagePath : "", atlasPath ? atlasPath : "" });
}

bool Engine::SwitchSkin(int index)
{
    if (index < 0 || index >= (int)skins.size())
        return false;

    const SkinDef& skin = skins[(size_t)index];
    if (!LoadPlayerAtlas(skin.imagePath.c_str(), skin.atlasPath.c_str()))
        return false;

    currentSkinIndex = index;
    return true;
}

bool Engine::SwitchSkinByName(const char* name)
{
    if (!name) return false;
    for (size_t i = 0; i < skins.size(); ++i)
        if (skins[i].name == name)
            return SwitchSkin((int)i);
    return false;
}

void Engine::NextSkin()
{
    if (skins.empty()) return;
    int next = (currentSkinIndex + 1) % (int)skins.size();
    SwitchSkin(next);
}

void Engine::PrevSkin()
{
    if (skins.empty()) return;
    int prev = (currentSkinIndex - 1 + (int)skins.size()) % (int)skins.size();
    SwitchSkin(prev);
}

const char* Engine::GetCurrentSkinName() const
{
    if (currentSkinIndex < 0 || currentSkinIndex >= (int)skins.size())
        return "";
    return skins[(size_t)currentSkinIndex].name.c_str();
}

void Engine::Run()
{
    while (window.process())
    {
        beginFrame();
        pollInputAndDebug();

        if (player.hp <= 0 && !isDead)
            Kill();

        updateDeathState(dt);

        if (!isDead)
            updateGameplay(dt);

        renderWorld(dt);
        renderHUD();
        endFrame();
    }
}

void Engine::Shutdown()
{
    for (Bullet& b : bullets) if (b.sprite) DestroySprite(b.sprite);
    bullets.clear();

    EventClear();
    DestroyAnimator(player.anim); DestroySprite(player.sprite);
    DestroySprite(background);
    DestroySound(sfxPunch); DestroySound(sfxJump); DestroySound(bgm);
    FreeAtlas(playeratlas); PL_FreeImage(playersheet);

#ifdef _DEBUG
    DebugPanel_Destroy();
#endif
}

// ============================================================
// Gameplay actions - callable independently of the frame loop
// ============================================================

void Engine::Jump()
{
    if (isDead || player.jumping || player.punching)
        return;

    player.baseY     = player.y;
    player.velocityY = player.jumpForce;
    player.jumping   = true;

    stamina -= STAMINA_JMP_DRAIN;
    if (stamina <= 0.0f)
    {
        stamina          = 0.0f;
        staminaExhausted = true;
    }

    Anim(player.anim, PLAYER, JUMP);
    lastAnimState = Player::State::JUMP;
    sfxJump->play(false);
    Event("player_jumped", &player);
}

void Engine::Punch()
{
    if (isDead || player.punching || player.punchCooldown > 0.0f || player.jumping)
        return;

    player.punching   = true;
    player.punchTimer = 0.0f;
    player.punchHit   = false;
    Anim(player.anim, PLAYER, PUNCH);
    lastAnimState = Player::State::PUNCH;
    sfxPunch->play(false);
    Event("punch_started", &player);
}

void Engine::Shoot()
{
    if (isDead || player.ammo <= 0 || player.shootCooldown > 0.0f)
        return;

    float pullDX  = -player.aimDirX; // aimDir already points from mouse -> player, invert for pull
    float pullDY  = -player.aimDirY;
    (void)pullDX; (void)pullDY;

    Bullet b;
    b.x    = player.muzzleX() - 24.0f;
    b.y    = player.muzzleY() - 24.0f;
    b.velX = player.aimDirX * MAX_POWER;
    b.velY = player.aimDirY * MAX_POWER;
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

void Engine::TakeDamage(float dmg)
{
    if (isDead) return;
    player.hp -= dmg;
    if (player.hp < 0) player.hp = 0;
}

void Engine::Kill()
{
    isDead         = true;
    respawnTimer   = RESPAWN_DELAY;
    deathAnimDone  = false;
    deathAnimTimer = 0.0f;
    Anim(player.anim, PLAYER, DIE);
    lastAnimState  = Player::State::IDLE;
}

void Engine::Respawn()
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
    stamina          = MAX_STAMINA;
    staminaExhausted = false;
    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    deathAnimDone  = false;
    deathAnimTimer = 0.0f;
    Anim(player.anim, PLAYER, IDLE);
    lastAnimState = Player::State::IDLE;
}

void Engine::SetSpawnPoint(float x, float y)
{
    SPAWN_X = x;
    SPAWN_Y = y;
}

// ============================================================
// Per-frame steps
// ============================================================

void Engine::beginFrame()
{
    dt = timer.delta();
    if (dt > 0.05f) dt = 0.05f;
    Sleep(10);

    fpsTimer += dt; ++fpsFrames;
    if (fpsTimer >= 1.0f)
    {
        currentFPS = fpsFrames / fpsTimer;
        fpsFrames  = 0; fpsTimer = 0.0f;
    }

    PollInput(&window);
    sw = window.getWidth();
    sh = window.getHeight();
    promptPulse += dt;
}

void Engine::pollInputAndDebug()
{
#ifdef _DEBUG
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
}

void Engine::updateDeathState(float dtSeconds)
{
    if (!isDead) return;

    respawnTimer -= dtSeconds;
    if (respawnTimer <= 0.0f)
        Respawn();
}

void Engine::updateGameplay(float dtSeconds)
{
    bool shiftHeld = KeyDown(VK_SHIFT) != 0;

    if (staminaExhausted && stamina >= MAX_STAMINA)
        staminaExhausted = false;

    int mousePixelX = 0, mousePixelY = 0;
    MousePos(&mousePixelX, &mousePixelY);

    float mouseWorldX, mouseWorldY;
    screenToWorld((float)mousePixelX, (float)mousePixelY,
                  main_cam, sw, sh, mouseWorldX, mouseWorldY);

    float pullLen = 0.0f;
    updateAiming(dtSeconds, mouseWorldX, mouseWorldY, pullLen);

    player.shootCooldown -= dtSeconds;
    player.punchCooldown -= dtSeconds;

    handleShootInput(dtSeconds, pullLen);
    handlePunchInput();
    handleSkinSwitchInput();

    bool  moving = false;
    float moveX = 0.0f, moveY = 0.0f;
    readMovementInput(moving, moveX, moveY);

    bool canRun = false;
    updateStamina(dtSeconds, shiftHeld, moving, canRun);

    handleJumpInput();
    tickPunch(dtSeconds);
    applyPhysics(dtSeconds, moveX, moveY, moving, canRun);
    updateCameraFollow(dtSeconds);
    updateBullets(dtSeconds);

    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
}

void Engine::updateAiming(float /*dtSeconds*/, float mouseWorldX, float mouseWorldY, float& pullLen)
{
    float pullDX = mouseWorldX - player.muzzleX();
    float pullDY = mouseWorldY - player.muzzleY();
    pullLen = sqrtf(pullDX * pullDX + pullDY * pullDY);

    if (pullLen > 0.001f)
    {
        player.aimDirX = -pullDX / pullLen;
        player.aimDirY = -pullDY / pullLen;
    }

    player.facingX = (pullDX >= 0.0f) ? 1.0f : -1.0f;
}

void Engine::handleShootInput(float /*dtSeconds*/, float pullLen)
{
    int mousePixelX = 0, mousePixelY = 0;
    MousePos(&mousePixelX, &mousePixelY);
    bool mouseInWindow = (mousePixelX >= 0 && mousePixelX < sw &&
                          mousePixelY >= 0 && mousePixelY < sh);

    bool mouseHeld   = (mouseInWindow && KeyDown(VK_LBUTTON)) != 0;
    bool justClicked = mouseHeld && !prevMouseHeld;
    prevMouseHeld    = mouseHeld;

    if (player.ammo > 0 && justClicked && player.shootCooldown <= 0.0f)
    {
        // Shoot() fires at MAX_POWER along aimDir; replicate original pull-based power here.
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
}

void Engine::handlePunchInput()
{
    int mousePixelX = 0, mousePixelY = 0;
    MousePos(&mousePixelX, &mousePixelY);
    bool mouseInWindow = (mousePixelX >= 0 && mousePixelX < sw &&
                          mousePixelY >= 0 && mousePixelY < sh);

    bool rMouseHeld   = (mouseInWindow && KeyDown(VK_RBUTTON)) != 0;
    bool rJustPressed = rMouseHeld && !prevRMouseHeld;
    prevRMouseHeld    = rMouseHeld;

    if (rJustPressed)
        Punch();
}

void Engine::handleSkinSwitchInput()
{
    if (skins.empty()) return;

    if (KeyPressed('T')) NextSkin();
    if (KeyPressed('Y')) PrevSkin();
}

void Engine::readMovementInput(bool& moving, float& moveX, float& moveY)
{
    moving = false;
    moveX  = 0.0f;
    moveY  = 0.0f;

    if (KeyDown(VK_RIGHT) || KeyDown('D')) { moveX =  1.0f; moving = true; }
    if (KeyDown(VK_LEFT)  || KeyDown('A')) { moveX = -1.0f; moving = true; }

    if (!player.jumping && !player.punching)
    {
        if (KeyDown(VK_UP)   || KeyDown('W')) { moveY = -1.0f; moving = true; }
        if (KeyDown(VK_DOWN) || KeyDown('S')) { moveY =  1.0f; moving = true; }
    }
}

void Engine::updateStamina(float dtSeconds, bool shiftHeld, bool moving, bool& canRun)
{
    canRun = shiftHeld && !staminaExhausted;

    if (canRun && moving)
    {
        stamina -= STAMINA_RUN_DRAIN * dtSeconds;
        if (stamina <= 0.0f)
        {
            stamina          = 0.0f;
            staminaExhausted = true;
            canRun           = false;
        }
    }
    else if (!shiftHeld || !moving)
    {
        stamina = std::min(stamina + STAMINA_RECOVER * dtSeconds, MAX_STAMINA);
    }
}

void Engine::handleJumpInput()
{
    if (KeyPressed(VK_SPACE))
        Jump();
}

void Engine::tickPunch(float dtSeconds)
{
    if (!player.punching) return;

    player.punchTimer += dtSeconds;

    if (player.punchTimer >= PUNCH_DURATION)
    {
        player.punching      = false;
        player.punchCooldown = PUNCH_COOLDOWN;
    }
}

void Engine::applyPhysics(float dtSeconds, float moveX, float moveY, bool moving, bool canRun)
{
    float currentSpeed = canRun ? player.speed * 1.7f : player.speed;

    if (!player.punching)
        player.x += moveX * currentSpeed * dtSeconds;

    if (!player.jumping)
    {
        if (!player.punching)
        {
            player.y += moveY * currentSpeed * dtSeconds;
            player.y  = std::max(player.minY, std::min(player.y, player.maxY - player.h));
        }
        player.baseY = player.y;
    }
    else
    {
        player.velocityY += player.gravity * dtSeconds;
        player.y         += player.velocityY * dtSeconds;
        if (player.y >= player.baseY)
        {
            player.y         = player.baseY;
            player.velocityY = 0.0f;
            player.jumping   = false;

            if (!player.punching)
                applyAnimationState(moving, canRun);

            Event("player_landed", &player);
        }
    }

    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {player.facingX, 1.0f};
    player.sprite->targetScale    = {player.facingX, 1.0f};

    SetAnimatorParent(player.anim, player.x, player.y, 0.0f, player.facingX, 1.0f);

    if (!player.jumping && !player.punching)
        applyAnimationState(moving, canRun);
}

void Engine::applyAnimationState(bool moving, bool canRun)
{
    if (moving && canRun && lastAnimState != Player::State::RUN)
    {
        Anim(player.anim, PLAYER, RUN);
        lastAnimState = Player::State::RUN;
    }
    else if (moving && !canRun && lastAnimState != Player::State::WALK)
    {
        Anim(player.anim, PLAYER, WALK);
        lastAnimState = Player::State::WALK;
    }
    else if (!moving && lastAnimState != Player::State::IDLE)
    {
        Anim(player.anim, PLAYER, IDLE);
        lastAnimState = Player::State::IDLE;
    }
}

void Engine::updateCameraFollow(float dtSeconds)
{
    main_cam.position.x += (player.x - (float)sw * 0.5f + 340.0f - main_cam.position.x) * 5.0f * dtSeconds;
    main_cam.position.y += (player.y - (float)sh * 0.5f + 280.0f - main_cam.position.y) * 5.0f * dtSeconds;
}

void Engine::updateBullets(float dtSeconds)
{
    for (Bullet& b : bullets)
    {
        if (b.dead) continue;
        b.velY += BULLET_GRAVITY * dtSeconds;
        b.x    += b.velX * dtSeconds;
        b.y    += b.velY * dtSeconds;
        b.sprite->position       = {b.x, b.y};
        b.sprite->targetPosition = {b.x, b.y};
        if (b.sprite) b.sprite->update(dtSeconds);
        b.life -= dtSeconds;
        if (b.life <= 0.0f) { b.dead = true; continue; }

        float phbX = player.x + (player.w - 100.0f) * 0.5f;
        float phbY = player.y + player.h;
        if (AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH, phbX, phbY, 100.0f, 200.0f))
        {
            b.dead = true;
            TakeDamage(BULLET_DAMAGE);
            Event("bullet_hit_player", nullptr, &b);
        }
    }

    for (Bullet& b : bullets)
        if (b.dead && b.sprite) { DestroySprite(b.sprite); b.sprite = nullptr; }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](const Bullet& b){ return b.dead; }), bullets.end());
}

// ============================================================
// Rendering
// ============================================================

void Engine::renderWorld(float dtSeconds)
{
    PL_Clear(0.12f, 0.12f, 0.18f, 1.0f);
    applyCamera2D(main_cam, sw, sh);

    drawRect(0, player.maxY, 2000, 64, 0.25f, 0.20f, 0.15f);

    background->update(dtSeconds); background->draw(main_cam);

    for (Bullet& b : bullets) if (b.sprite) b.sprite->draw(main_cam);

    if (!isDead)
    {
        player.sprite->update(dtSeconds); player.sprite->draw(main_cam);
        TickAnimator(player.anim, dtSeconds, playersheet, playeratlas, &main_cam);
    }
    else
    {
        SetAnimatorParent(player.anim, player.x, player.y, 0.0f, player.facingX, 1.0f);
        if (!deathAnimDone)
        {
            deathAnimTimer += dtSeconds;
            TickAnimator(player.anim, dtSeconds, playersheet, playeratlas, &main_cam);
            if (deathAnimTimer >= DEATH_ANIM_DUR)
                deathAnimDone = true;
        }
        else
        {
            TickAnimator(player.anim, 0.0f, playersheet, playeratlas, &main_cam);
        }
    }

    if (!isDead)
    {
        float dbgX = player.x + (player.w - 100.0f) * 0.5f, dbgY = player.y + player.h;
        float dbgW = player.jumping ? 60.0f : 100.0f, dbgH = player.jumping ? 100.0f : 200.0f;
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

    {
        const float cs = 12.0f;
        drawLine(SPAWN_X - cs, SPAWN_Y, SPAWN_X + cs, SPAWN_Y, 0.2f, 1.0f, 0.4f, 0.8f, 1.5f);
        drawLine(SPAWN_X, SPAWN_Y - cs, SPAWN_X, SPAWN_Y + cs, 0.2f, 1.0f, 0.4f, 0.8f, 1.5f);
    }

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
}

void Engine::renderHUD()
{
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

    // Stamina bar
    if (!isDead)
    {
        float stFill = std::max(0.0f, std::min(stamina / MAX_STAMINA, 1.0f));
        float sr = 1.0f;
        float sg = staminaExhausted ? 0.15f : (0.3f + stFill * 0.7f);
        float sb = staminaExhausted ? 0.05f : (stFill * 0.2f);

        const float barW = 220.0f, barH = 14.0f;
        const float bx   = 20.0f,  by   = (float)sh - 90.0f;

        UI::Label("ST", bx, by, 2.0f,
                  sr, staminaExhausted ? 0.2f : 0.85f, 0.1f, 1.0f);
        UI::ProgressBar(stFill, bx, by + 16.0f, barW, barH,
                        sr, sg, sb, 0.15f, 0.15f, 0.15f);
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
}

void Engine::endFrame()
{
    PL_Present(&window);
}
