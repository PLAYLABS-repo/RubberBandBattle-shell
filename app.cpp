#include "PlaylabsGL.h"
#include "Src/UI.h"
#include <GL/gl.h>
#include <cstdio>
#include <algorithm>
#include <windows.h>

#include "DrawHelpers.h"
#include "Player.h"
// Build 0.0.19

int main()
{
    Window window;
    if (!window.create("RubberBandBattle-Shell", 1280, 720))
        return -1;

    // ---- Font ----
    UI::_font::load("Resources/Font/Confale.ttf");

    Camera main_cam;
    main_cam.position = {0.0f, 0.0f};
    main_cam.zoom     = 1.723f;

    Timer timer;
#ifdef _DEBUG
    float fpsTimer = 0.0f, currentFPS = 0.0f;
    int   fpsFrames = 0;
#endif

    Player player;
    // ---- Assets ----
    Image* playersheet = PL_LoadImage("Resources/Skins/spritemap.png");
    Atlas* playeratlas = LoadAtlas("Resources/Skins/spritemap.json");
    player.anim = CreateAnimator();
    player.anim->load("Resources/Skins/Animation.json");
    Image* ground =PL_LoadImage("Resources/Texture/BGDebug.png");

    player.sprite                 = CreateSprite();
    player.sprite->image          = playersheet;
    player.sprite->atlas          = playeratlas;
    player.sprite->frameName      = "0001";
    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {1.0f, 1.0f};

    Anim(player.anim, PLAYER, IDLE);
    Sound* bgm =  CreateSound();
    bgm->load("bgm.wav");
    Sprite* background         = CreateSprite();
    background->image          = ground;
    background->position       = (0.0f,0.0f);
    background->skewX          = 0.0f;


    const AABB wallBox(100.0f, 100.0f, 100.0f, 100.0f);

    const float SPAWN_X = 200.0f;
    const float SPAWN_Y = 436.0f;
    player.resetToSpawn(SPAWN_X, SPAWN_Y);
    bgm->play(true);
    while (window.process())
    {
        float dt = timer.delta();
        if (dt > 0.05f) dt = 0.05f;
        Sleep(10);

#ifdef _DEBUG
        fpsTimer += dt; ++fpsFrames;
        if (fpsTimer >= 1.0f)
        {
            currentFPS = fpsFrames / fpsTimer;
            fpsFrames  = 0; fpsTimer = 0.0f;
        }
#endif

        Player::Input in = Player::gatherInput(window);
        int sw = window.getWidth(), sh = window.getHeight();

        player.update(dt, in, wallBox);

        main_cam.position.x += (player.x - (float)sw * 0.5f + 340.0f - main_cam.position.x) * 5.0f * dt;
        main_cam.position.y += (player.y - (float)sh * 0.5f + 280.0f - main_cam.position.y) * 5.0f * dt;

        // ======================================================
        // RENDER — world
        // ======================================================
        PL_Clear(0.12f, 0.12f, 0.18f, 1.0f);
        applyCamera2D(main_cam, sw, sh);



        background->update(dt); background->draw(main_cam);

        player.sprite->update(dt); player.sprite->draw(main_cam);
        TickAnimator(player.anim, dt, playersheet, playeratlas, &main_cam);
float screenX = (player.x - main_cam.position.x - 580) * main_cam.zoom + sw * 0.5f;
float screenY = (player.y - main_cam.position.y + -300) * main_cam.zoom + sh * 0.5f;
        // ======================================================
        // RENDER — HUD
        // ======================================================
        applyScreenSpace(sw, sh);
        UI::BeginFrame(sw, sh);
#ifdef _DEBUG
        {
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS  %.1f", currentFPS);
            UI::Label(fpsText, (float)sw - UI::_font::textWidth(fpsText, 2.0f) - 14.0f, 18.0f,
                      2.0f, 0.3f, 1.0f, 0.3f);
        }
#endif
        // Stamina bar
        UI::Label("Player",
          screenX - UI::_font::textWidth("Player", 2.0f) * 0.5f,
          screenY - 30.0f,
          2.0f,
          1.0f, 1.0f, 0.0f);
        {
            float stFill = std::max(0.0f, std::min(player.stamina / Player::MAX_STAMINA, 1.0f));
            float sr = 1.0f;
            float sg = player.staminaExhausted ? 0.15f : (0.3f + stFill * 0.7f);
            float sb = player.staminaExhausted ? 0.05f : (stFill * 0.2f);

            const float barW = 220.0f, barH = 14.0f;
            const float bx   = 20.0f,  by   = (float)sh - 90.0f;

            UI::Label("Stamina", bx, by, 2.0f,
                      sr, player.staminaExhausted ? 0.2f : 0.85f, 0.1f, 1.0f);
            UI::ProgressBar(stFill, bx, by + 16.0f, barW, barH,
                            sr, sg, sb, 0.15f, 0.15f, 0.15f);
        }

        UI::EndFrame();
        PL_Present(&window);
    }

    DestroySound(bgm);
    DestroyAnimator(player.anim); DestroySprite(player.sprite);
    DestroySprite(background);
    FreeAtlas(playeratlas);       PL_FreeImage(playersheet);
    return 0;
}
