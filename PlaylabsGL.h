#pragma once
// =============================================================
// PlaylabsGL — Unified Public API (static build)
// Compile PlaylabsGL.cpp alongside your other sources.
// =============================================================
#include "Src/Vec2.h"
#include "Src/AABB.h"
#include "Src/Image.h"
#include "Src/Atlas.h"
#include "Src/Camera.h"
#include "Src/Sprite.h"
#include "Src/Sound.h"
#include "Src/Input.h"
#include "Src/Timer.h"
#include "Src/Window.h"
#include "Src/TimelineAnimator.h"

#ifdef __cplusplus
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>    // std::remove_if

// =============================================================
// EventData — payload carried by every emission
// =============================================================
struct EventData
{
    const char* name;
    void*       sender;
    void*       payload;

    // Explicit constructor — GCC 6 does not support default member
    // initialisers (= nullptr) on non-const members in all contexts.
    EventData(const char* n, void* s, void* p)
        : name(n), sender(s), payload(p) {}
};

// C++11 compatible — avoid 'using' alias at namespace scope inside header
typedef std::function<void(const EventData&)> EventCallback;

// =============================================================
// EventBus — lightweight static pub/sub
//
// The two static data members are DEFINED in PlaylabsGL.cpp so
// there is exactly one definition across all translation units.
// This avoids the C++17 'inline static' extension that GCC 6
// does not support.
//
//   int h = EventBus::Subscribe("player_died", myLambda);
//   EventBus::Emit("player_died", this, &info);
//   EventBus::Unsubscribe("player_died", h);
// =============================================================
struct EventBus
{
    struct Entry
    {
        int           id;
        EventCallback cb;
    };

    // Spell out the closing '> >' — some GCC 6 configs warn on '>>'
    typedef std::unordered_map<std::string, std::vector<Entry> > ListenerMap;

    static int  Subscribe  (const std::string& name, EventCallback cb);
    static void Unsubscribe(const std::string& name, int id);
    static void Emit       (const char* name,
                            void*       sender  = nullptr,
                            void*       payload = nullptr);
    static void Clear      ();

    // Storage lives in PlaylabsGL.cpp
    static ListenerMap s_listeners;
    static int         s_nextId;
};

// =============================================================
// Event(name, ...)  — sugar over EventBus::Emit
//
//   Event("player_died");
//   Event("enemy_hit", this, &hitInfo);
// =============================================================
#define Event(name, ...) EventBus::Emit(name, ##__VA_ARGS__)

#endif // __cplusplus

extern "C"
{
    // ---- Event (C bindings) ---------------------------------
    typedef void (*CEventCallback)(const char* name, void* sender, void* payload);

    int  EventSubscribe  (const char* name, CEventCallback cb);
    void EventUnsubscribe(const char* name, int handle);
    void EventEmit       (const char* name, void* sender, void* payload);
    void EventClear      ();

    // ---- Assets ---------------------------------------------
    // "LoadImage" is #defined to LoadImageA/W by <windows.h>.
    // Prefix PL_ avoids the collision without breaking callers
    // (main.cpp uses the PL_ names too).
    Image*  PL_LoadImage(const char* path);
    void    PL_FreeImage(Image* img);
    Atlas*  LoadAtlas   (const char* path);
    void    FreeAtlas   (Atlas* atlas);

    // ---- Sound ----------------------------------------------
    Sound*  CreateSound ();
    void    DestroySound(Sound* snd);

    // ---- Sprite ---------------------------------------------
    Sprite* CreateSprite ();
    void    DestroySprite(Sprite* spr);

    // ---- TimelineAnimator -----------------------------------
    TimelineAnimator* CreateAnimator ();
    void              DestroyAnimator(TimelineAnimator* anim);

    /// Internal — use the Anim(anim, ENTITY, CLIP) macro instead.
    void AnimPlay(TimelineAnimator* anim, const char* entity, const char* clip);

    void SetAnimatorParent(
        TimelineAnimator* anim,
        float x, float y,
        float rotationRadians,
        float scaleX, float scaleY
    );
    void ClearAnimatorParent(TimelineAnimator* anim);

    /// Update + draw an animator in one call.
    void TickAnimator(
        TimelineAnimator* anim,
        float dt,
        Image* img, Atlas* atlas, Camera* cam
    );

    // ---- Camera ---------------------------------------------
    void ApplyCamera(Camera* cam, int screenWidth, int screenHeight);

    // ---- Window / GL ----------------------------------------
    // "Clear" and "Present" are common Win32/GL macro names;
    // prefix PL_ keeps them unambiguous.
    void PL_Present(Window* win);
    void PL_Clear  (float r, float g, float b, float a);

    // ---- Input ----------------------------------------------
    void PollInput  (Window* win);
    int  KeyDown    (int vkey);
    int  KeyPressed (int vkey);
    void MousePos   (int* x, int* y);

    // ---- AABB -----------------------------------------------
    int  AABBIntersects(
        float ax, float ay, float aw, float ah,
        float bx, float by, float bw, float bh
    );
    int  AABBContains(
        float bx, float by, float bw, float bh,
        float px, float py
    );

} // extern "C"

// =============================================================
// Anim(anim, ENTITY, CLIP)
//
//   Anim(playerAnim, PLAYER, RUN)
//   Anim(playerAnim, PLAYER, IDLE)
//   Anim(playerAnim, PLAYER, JUMP)
//
// Expands to: AnimPlay(playerAnim, "PLAYER", "RUN")
// which calls anim->play("PLAYER", "RUN")
// which looks up "PLAYER_ANIM_RUN" in the loaded JSON.
// =============================================================
#define Anim(anim, entity, clip) AnimPlay((anim), #entity, #clip)
