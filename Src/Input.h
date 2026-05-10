#pragma once
#include <windows.h>

class Input
{
public:
    static void update(HWND hwnd);
    static bool isKeyDown(int key);
    static bool isKeyPressed(int key);

    static int mouseX;
    static int mouseY;

private:
    static bool keys[256];
    static bool prevKeys[256];
};
