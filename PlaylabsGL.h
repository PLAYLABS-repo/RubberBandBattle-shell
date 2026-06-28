#pragma once
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
    Image*  Playlabs_LoadImage(const char* path);
    void    Playlabs_FreeImage(Image* img);
    Atlas*  Playlabs_LoadAtlas(const char* path);
    void    Playlabs_FreeAtlas(Atlas* atlas);
    Sound*  Playlabs_CreateSound();
    void    Playlabs_DestroySound(Sound* snd);
    Sprite* Playlabs_CreateSprite();
    void    Playlabs_DestroySprite(Sprite* spr);
    TimelineAnimator* Playlabs_CreateAnimator();
    void              Playlabs_DestroyAnimator(TimelineAnimator* anim);
    void Playlabs_AnimPlay(TimelineAnimator* anim, const char* entity, const char* clip);
    void Playlabs_SetAnimatorParent(TimelineAnimator* anim, float x, float y, float rotationRadians, float scaleX, float scaleY);
    void Playlabs_ClearAnimatorParent(TimelineAnimator* anim);
    void Playlabs_TickAnimator(TimelineAnimator* anim, float dt, Image* img, Atlas* atlas, Camera* cam);
    void Playlabs_ApplyCamera(Camera* cam, int screenWidth, int screenHeight);
    void Playlabs_Present(Window* win);
    void Playlabs_Clear(float r, float g, float b, float a);
    void Playlabs_PollInput(Window* win);
    int  Playlabs_KeyDown(int vkey);
    int  Playlabs_KeyPressed(int vkey);
    void Playlabs_MousePos(int* x, int* y);
    int  Playlabs_AABBIntersects(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh);
    int  Playlabs_AABBContains(float bx, float by, float bw, float bh, float px, float py);
}

#define Playlabs_Anim(anim, entity, clip) \
    Playlabs_AnimPlay((anim), #entity, #clip)

// =============================================================
// Short aliases
// =============================================================
inline Image*  PL_LoadImage(const char* p)                    { return Playlabs_LoadImage(p); }
inline void    PL_FreeImage(Image* i)                         { Playlabs_FreeImage(i); }
inline Atlas*  LoadAtlas(const char* p)                       { return Playlabs_LoadAtlas(p); }
inline void    FreeAtlas(Atlas* a)                            { Playlabs_FreeAtlas(a); }
inline Sound*  CreateSound()                                  { return Playlabs_CreateSound(); }
inline void    DestroySound(Sound* s)                         { Playlabs_DestroySound(s); }
inline Sprite* CreateSprite()                                 { return Playlabs_CreateSprite(); }
inline void    DestroySprite(Sprite* s)                       { Playlabs_DestroySprite(s); }
inline TimelineAnimator* CreateAnimator()                     { return Playlabs_CreateAnimator(); }
inline void    DestroyAnimator(TimelineAnimator* a)           { Playlabs_DestroyAnimator(a); }
inline void    SetAnimatorParent(TimelineAnimator* a,
                   float x, float y, float r, float sx, float sy) { Playlabs_SetAnimatorParent(a,x,y,r,sx,sy); }
inline void    TickAnimator(TimelineAnimator* a, float dt,
                   Image* img, Atlas* atl, Camera* cam)       { Playlabs_TickAnimator(a,dt,img,atl,cam); }
#define Anim(anim, entity, clip) Playlabs_Anim(anim, entity, clip)
inline void    PollInput(Window* w)                           { Playlabs_PollInput(w); }
inline int     KeyDown(int v)                                 { return Playlabs_KeyDown(v); }
inline int     KeyPressed(int v)                              { return Playlabs_KeyPressed(v); }
inline void    MousePos(int* x, int* y)                       { Playlabs_MousePos(x, y); }
inline void    PL_Clear(float r,float g,float b,float a)      { Playlabs_Clear(r,g,b,a); }
inline void    PL_Present(Window* w)                          { Playlabs_Present(w); }

inline int     AABBIntersects(float ax,float ay,float aw,float ah,
                   float bx,float by,float bw,float bh)       { return Playlabs_AABBIntersects(ax,ay,aw,ah,bx,by,bw,bh); }

// =============================================================
// EventData / EventBus / Event / EventClear
// =============================================================
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

struct EventData
{
    std::string name;
    void*       sender  = nullptr;
    void*       payload = nullptr;
};

struct EventBus
{
    using Listener = std::function<void(const EventData&)>;

    struct Entry
    {
        std::string name;
        int         handle;
        Listener    fn;
    };

    static std::vector<Entry>& _listeners()
    {
        static std::vector<Entry> v;
        return v;
    }
    static int& _nextHandle()
    {
        static int h = 0;
        return h;
    }

    static int Subscribe(const std::string& name, Listener fn)
    {
        int h = ++_nextHandle();
        _listeners().push_back({name, h, fn});
        return h;
    }

    static void Unsubscribe(const std::string& name, int handle)
    {
        auto& v = _listeners();
        v.erase(std::remove_if(v.begin(), v.end(),
            [&](const Entry& e){ return e.handle == handle; }), v.end());
    }

    static void Emit(const std::string& name,
                     void* sender = nullptr, void* payload = nullptr)
    {
        EventData data{name, sender, payload};
        for (auto& e : _listeners())
            if (e.name == name || e.name == "*")
                e.fn(data);
    }

    static void Clear()
    {
        _listeners().clear();
    }
};

inline void Event(const char* name, void* sender = nullptr, void* payload = nullptr)
{
    EventBus::Emit(name, sender, payload);
}

inline void EventClear()
{
    EventBus::Clear();
}
