// PlaylabsGL.cpp — compile alongside your other .cpp sources
#include "PlaylabsGL.h"
#include <GL/gl.h>
#include <windows.h>

// =============================================================
// EventBus — static member definitions (one TU only)
// =============================================================
EventBus::ListenerMap EventBus::s_listeners;
int                   EventBus::s_nextId = 0;

int EventBus::Subscribe(const std::string& name, EventCallback cb)
{
    int id = s_nextId++;
    Entry e;
    e.id = id;
    e.cb = cb;
    s_listeners[name].push_back(e);
    return id;
}

void EventBus::Unsubscribe(const std::string& name, int id)
{
    ListenerMap::iterator it = s_listeners.find(name);
    if (it == s_listeners.end()) return;
    std::vector<Entry>& vec = it->second;
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [id](const Entry& e){ return e.id == id; }),
        vec.end()
    );
}

void EventBus::Emit(const char* name, void* sender, void* payload)
{
    if (!name) return;
    ListenerMap::iterator it = s_listeners.find(name);
    if (it == s_listeners.end()) return;
    EventData data(name, sender, payload);
    std::vector<Entry> copy = it->second;   // snapshot — safe mid-fire unsub
    for (size_t i = 0; i < copy.size(); ++i)
        copy[i].cb(data);
}

void EventBus::Clear()
{
    s_listeners.clear();
}

extern "C"
{

// =============================================================
// EventBus — C bindings
// =============================================================

int EventSubscribe(const char* name, CEventCallback cb)
{
    if (!name || !cb) return -1;
    return EventBus::Subscribe(name, [cb](const EventData& d){
        cb(d.name, d.sender, d.payload);
    });
}

void EventUnsubscribe(const char* name, int handle)
{
    if (name) EventBus::Unsubscribe(name, handle);
}

void EventEmit(const char* name, void* sender, void* payload)
{
    if (name) EventBus::Emit(name, sender, payload);
}

void EventClear() { EventBus::Clear(); }

// =============================================================
// Image
// =============================================================

Image* PL_LoadImage(const char* path)
{
    if (!path) return nullptr;
    Image* img = new Image();
    if (!img->load(path)) { delete img; return nullptr; }
    return img;
}

void PL_FreeImage(Image* img) { delete img; }

// =============================================================
// Atlas
// =============================================================

Atlas* LoadAtlas(const char* path)
{
    if (!path) return nullptr;
    Atlas* a = new Atlas();
    if (!a->load(path)) { delete a; return nullptr; }
    return a;
}

// Convenience: Load atlas from a TexturePacker-formatted file
Atlas* LoadAtlasTP(const char* path)
{
    if (!path) return nullptr;
    Atlas* a = new Atlas();
    if (!a->load(path)) { delete a; return nullptr; }
    return a;
}

void FreeAtlas(Atlas* atlas) { delete atlas; }

int AtlasGetFrame(Atlas* atlas, const char* name, float* x, float* y, float* w, float* h)
{
    if (!atlas || !name) return 0;
    Frame f;
    if (atlas->get(name, f)) {
        if (x) *x = f.x;
        if (y) *y = f.y;
        if (w) *w = f.w;
        if (h) *h = f.h;
        return 1;
    }
    return 0;
}

// =============================================================
// Sound
// =============================================================

Sound* CreateSound()          { return new Sound(); }
void   DestroySound(Sound* s) { delete s; }

// =============================================================
// Sprite
// =============================================================

Sprite* CreateSprite()           { return new Sprite(); }
void    DestroySprite(Sprite* s) { delete s; }

// =============================================================
// TimelineAnimator
// =============================================================

TimelineAnimator* CreateAnimator()                     { return new TimelineAnimator(); }
void              DestroyAnimator(TimelineAnimator* a)  { delete a; }

void AnimPlay(TimelineAnimator* anim, const char* entity, const char* clip)
{
    if (!anim || !entity || !clip) return;
    anim->play(entity, clip);
}

void SetAnimatorParent(
    TimelineAnimator* anim,
    float x, float y,
    float rotationRadians,
    float scaleX, float scaleY
)
{
    if (!anim) return;
    anim->parent.enabled   = true;
    anim->parent.position  = {x, y};
    anim->parent.rotation  = rotationRadians;
    anim->parent.scale     = {scaleX, scaleY};
}

void ClearAnimatorParent(TimelineAnimator* anim)
{
    if (!anim) return;
    anim->parent.enabled   = false;
    anim->parent.position  = {0.0f, 0.0f};
    anim->parent.rotation  = 0.0f;
    anim->parent.scale     = {1.0f, 1.0f};
}

void TickAnimator(
    TimelineAnimator* anim,
    float dt,
    Image* img,
    Atlas* atlas,
    Camera* cam
)
{
    if (!anim || !img || !atlas)
        return;

    anim->update(dt);

    if (cam)
        anim->draw(img, atlas, *cam);
    else
    {
        Camera defaultCam;
        defaultCam.position = {0, 0};
        defaultCam.zoom = 1.0f;

        anim->draw(img, atlas, defaultCam);
    }
}

// =============================================================
// Camera
// =============================================================

void ApplyCamera(Camera* cam, int sw, int sh)
{
    if (cam) cam->apply(sw, sh);
}

// =============================================================
// Window / GL
// =============================================================

void PL_Present(Window* win)
{
    if (win) SwapBuffers(win->getHDC());
}

void PL_Clear(float r, float g, float b, float a)
{
    glViewport(0, 0, 1920, 1080);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1920, 1080, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// =============================================================
// Input
// =============================================================

void PollInput(Window* win)
{
    if (win) Input::update(win->getHWND());
}

int  KeyDown   (int vkey) { return Input::isKeyDown(vkey)    ? 1 : 0; }
int  KeyPressed(int vkey) { return Input::isKeyPressed(vkey) ? 1 : 0; }

void MousePos(int* x, int* y)
{
    if (x) *x = Input::mouseX;
    if (y) *y = Input::mouseY;
}

// =============================================================
// AABB
// =============================================================

int AABBIntersects(
    float ax, float ay, float aw, float ah,
    float bx, float by, float bw, float bh
)
{
    return AABB(ax, ay, aw, ah).intersects(AABB(bx, by, bw, bh)) ? 1 : 0;
}

int AABBContains(
    float bx, float by, float bw, float bh,
    float px, float py
)
{
    return AABB(bx, by, bw, bh).contains(px, py) ? 1 : 0;
}

} // extern "C"
