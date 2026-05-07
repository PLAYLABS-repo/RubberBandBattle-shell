#pragma once

#ifdef _WIN32
    #ifdef ENGINE_BUILD_DLL
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif
#else
    #define ENGINE_API
#endif

#include <windows.h>

extern "C" {

// CORE
ENGINE_API bool Engine_Init(const char* title, int width, int height);
ENGINE_API void Engine_PollEvents(); // MUST MATCH CPP
ENGINE_API void Engine_BeginFrame();
ENGINE_API void Engine_EndFrame();

// TIME
ENGINE_API float Engine_GetDeltaTime();

// INPUT
ENGINE_API bool Engine_IsKeyDown(int key);
ENGINE_API bool Engine_IsKeyPressed(int key);

// WINDOW
ENGINE_API int Engine_GetWidth();
ENGINE_API int Engine_GetHeight();
ENGINE_API void Engine_SetSize(int w, int h);

// CAMERA
ENGINE_API void Engine_SetCamera(float x, float y, float zoom);

// IMAGE
ENGINE_API void* Engine_LoadImage(const char* path);
ENGINE_API void Engine_DrawImage(void* img, float x, float y);
ENGINE_API void Engine_DeleteImage(void* img);

// ATLAS
ENGINE_API void* Engine_LoadAtlas(const char* path);
ENGINE_API void Engine_DeleteAtlas(void* a);

// TIMELINE
ENGINE_API void* Engine_CreateTimeline(const char* jsonPath);
ENGINE_API void Engine_UpdateTimeline(void* t);
ENGINE_API void Engine_PlayTimeline(void* t, const char* entity, const char* animType);
ENGINE_API void Engine_DeleteTimeline(void* t);

// SOUND
ENGINE_API void* Engine_LoadSound(const char* path);
ENGINE_API void Engine_PlaySound(void* s);
ENGINE_API void Engine_DeleteSound(void* s);

// LOG
ENGINE_API void Engine_Log(const char* msg);

}
