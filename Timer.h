#pragma once
#include <windows.h>

class Timer
{
private:
    LARGE_INTEGER freq;
    LARGE_INTEGER last;

public:
    Timer()
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last);
    }

    float delta()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        float dt = (float)(now.QuadPart - last.QuadPart) / (float)freq.QuadPart;
        last = now;

        return dt;
    }

    DWORD millis()
    {
        return GetTickCount();
    }
};
