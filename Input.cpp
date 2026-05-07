#include "Input.h"

bool Input::keys[256] = {};
bool Input::prevKeys[256] = {};

int Input::mouseX = 0;
int Input::mouseY = 0;

void Input::update(HWND hwnd)
{
    for (int i = 0; i < 256; i++)
        prevKeys[i] = keys[i];

    for (int i = 0; i < 256; i++)
        keys[i] = GetAsyncKeyState(i) & 0x8000;

    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);

    mouseX = p.x;
    mouseY = p.y;
}

bool Input::isKeyDown(int key)
{
    return keys[key];
}

bool Input::isKeyPressed(int key)
{
    return keys[key] && !prevKeys[key];
}
