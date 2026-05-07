#define ENGINE_BUILD_DLL

#include "Engine.h"

#include "Window.h"
#include "Input.h"
#include "Camera.h"
#include "Image.h"
#include "Atlas.h"
#include "TimelineAnimator.h"
#include "Timer.h"
#include "Sound.h"

#include <GL/gl.h>
#include <iostream>

// GLOBALS
static Window g_window;
static Timer  g_timer;
static Camera g_camera;

extern "C" {

ENGINE_API bool Engine_Init(const char* title, int width, int height)
{
    return g_window.create(title, width, height);
}

// ✅ FIXED: matches header
ENGINE_API void Engine_PollEvents()
{
    Input::update(g_window.getHWND());
}

ENGINE_API void Engine_BeginFrame()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

ENGINE_API void Engine_EndFrame()
{
    g_window.process();
}

ENGINE_API float Engine_GetDeltaTime()
{
    return g_timer.delta();
}

// INPUT
ENGINE_API bool Engine_IsKeyDown(int key)
{
    return Input::isKeyDown(key);
}

ENGINE_API bool Engine_IsKeyPressed(int key)
{
    return Input::isKeyPressed(key);
}

// WINDOW
ENGINE_API int Engine_GetWidth() { return g_window.getWidth(); }
ENGINE_API int Engine_GetHeight() { return g_window.getHeight(); }
ENGINE_API void Engine_SetSize(int w, int h) { g_window.setSize(w, h); }

// CAMERA
ENGINE_API void Engine_SetCamera(float x, float y, float zoom)
{
    g_camera.position = {x, y};
    g_camera.zoom = zoom;
}

// IMAGE
ENGINE_API void* Engine_LoadImage(const char* path)
{
    Image* img = new Image();
    img->load(path);
    return img;
}

ENGINE_API void Engine_DrawImage(void* img, float x, float y)
{
    Image* i = (Image*)img;
    if (i) i->bind();
}

ENGINE_API void Engine_DeleteImage(void* img)
{
    delete (Image*)img;
}

// ATLAS
ENGINE_API void* Engine_LoadAtlas(const char* path)
{
    Atlas* a = new Atlas();
    a->load(path);
    return a;
}

ENGINE_API void Engine_DeleteAtlas(void* a)
{
    delete (Atlas*)a;
}

// TIMELINE
ENGINE_API void* Engine_CreateTimeline(const char* jsonPath)
{
    TimelineAnimator* t = new TimelineAnimator();
    t->load(jsonPath);
    return t;
}

ENGINE_API void Engine_UpdateTimeline(void* t)
{
    TimelineAnimator* anim = (TimelineAnimator*)t;
    if (anim) anim->update(g_timer.delta());
}

ENGINE_API void Engine_PlayTimeline(void* t, const char* entity, const char* animType)
{
    TimelineAnimator* anim = (TimelineAnimator*)t;
    if (anim) anim->play(entity, animType);
}

ENGINE_API void Engine_DeleteTimeline(void* t)
{
    delete (TimelineAnimator*)t;
}

// SOUND
ENGINE_API void* Engine_LoadSound(const char* path)
{
    Sound* s = new Sound();
    s->load(path);
    return s;
}

ENGINE_API void Engine_PlaySound(void* s)
{
    Sound* snd = (Sound*)s;
    if (snd) snd->play();
}

ENGINE_API void Engine_DeleteSound(void* s)
{
    delete (Sound*)s;
}

// LOG
ENGINE_API void Engine_Log(const char* msg)
{
    std::cout << "[Engine] " << msg << std::endl;
}

}
