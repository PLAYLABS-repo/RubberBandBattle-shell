#pragma once

#ifdef _WIN32
    #ifdef PLAYFRAMEWORK_EXPORTS
        #define PLAY_API __declspec(dllexport)
    #else
        #define PLAY_API __declspec(dllimport)
    #endif
#else
    #define PLAY_API
#endif
