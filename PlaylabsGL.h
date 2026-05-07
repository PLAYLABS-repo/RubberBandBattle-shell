#pragma once

// =============================================================
// PlaylabsGL — Unified Public API (static build)
// Compile PlaylabsGL.cpp alongside your other sources.
// =============================================================

#include "Vec2.h"
#include "AABB.h"
#include "Image.h"
#include "Atlas.h"
#include "Camera.h"
#include "Sprite.h"
#include "Sound.h"
#include "Input.h"
#include "Timer.h"
#include "Window.h"
#include "TimelineAnimator.h"

extern "C"
{
    // ---- Assets ---------------------------------------------
    Image*  Playlabs_LoadImage(const char* path);
    void    Playlabs_FreeImage(Image* img);
    Atlas*  Playlabs_LoadAtlas(const char* path);
    void    Playlabs_FreeAtlas(Atlas* atlas);

    // ---- Sound ----------------------------------------------
    Sound*  Playlabs_CreateSound();
    void    Playlabs_DestroySound(Sound* snd);

    // ---- Sprite ---------------------------------------------
    Sprite* Playlabs_CreateSprite();
    void    Playlabs_DestroySprite(Sprite* spr);

    // ---- TimelineAnimator -----------------------------------
    TimelineAnimator* Playlabs_CreateAnimator();
    void              Playlabs_DestroyAnimator(TimelineAnimator* anim);

    /// Internal function — do not call directly.
    /// Use the Playlabs_Anim(anim, ENTITY, CLIP) macro instead.
    void Playlabs_AnimPlay(
        TimelineAnimator* anim,
        const char* entity,
        const char* clip
    );

    void Playlabs_SetAnimatorParent(
        TimelineAnimator* anim,
        float x, float y,
        float rotationRadians,
        float scaleX, float scaleY
    );

    void Playlabs_ClearAnimatorParent(TimelineAnimator* anim);

    /// Update + draw an animator in one call.
    void Playlabs_TickAnimator(
        TimelineAnimator* anim,
        float dt,
        Image* img, Atlas* atlas, Camera* cam
    );

    // ---- Camera ---------------------------------------------
    void Playlabs_ApplyCamera(Camera* cam, int screenWidth, int screenHeight);

    // ---- Window ---------------------------------------------
    void Playlabs_Present(Window* win);
    void Playlabs_Clear(float r, float g, float b, float a);

    // ---- Input ----------------------------------------------
    void Playlabs_PollInput(Window* win);
    int  Playlabs_KeyDown(int vkey);
    int  Playlabs_KeyPressed(int vkey);
    void Playlabs_MousePos(int* x, int* y);

    // ---- AABB -----------------------------------------------
    int  Playlabs_AABBIntersects(
        float ax, float ay, float aw, float ah,
        float bx, float by, float bw, float bh
    );
    int  Playlabs_AABBContains(
        float bx, float by, float bw, float bh,
        float px, float py
    );

} // extern "C"

// =============================================================
// Playlabs_Anim(anim, ENTITY, CLIP)
//
// Stringifies ENTITY and CLIP so you write plain tokens:
//
//   Playlabs_Anim(playerAnim, PLAYER, RUN)
//   Playlabs_Anim(playerAnim, PLAYER, IDLE)
//   Playlabs_Anim(playerAnim, PLAYER, JUMP)
//
// Expands to: Playlabs_AnimPlay(playerAnim, "PLAYER", "RUN")
// which calls: anim->play("PLAYER", "RUN")
// which looks up symbol "PLAYER_ANIM_RUN" in the loaded JSON.
// =============================================================
#define Playlabs_Anim(anim, entity, clip) \
    Playlabs_AnimPlay((anim), #entity, #clip)
