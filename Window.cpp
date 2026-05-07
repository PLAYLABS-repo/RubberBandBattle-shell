#include "Window.h"
#include <GL/gl.h>

static Window* g_window = nullptr;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            if (g_window)
            {
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);

                g_window->setSize(w, h); // ✅ FIXED
                glViewport(0, 0, w, h);
            }
            return 0;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

Window::Window()
    : hwnd(nullptr), hdc(nullptr), hrc(nullptr),
      running(false), width(0), height(0)
{}

Window::~Window()
{
    if (hrc)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(hrc);
    }

    if (hdc && hwnd)
        ReleaseDC(hwnd, hdc);

    if (hwnd)
        DestroyWindow(hwnd);
}

bool Window::create(const char* title, int w, int h)
{
    width = w;
    height = h;

    g_window = this;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "playFrameworkWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );

    if (!hwnd) return false;

    hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) return false;

    if (!SetPixelFormat(hdc, pf, &pfd))
        return false;

    hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hrc);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, width, height); // ✅ important

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    running = true;
    return true;
}

bool Window::process()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return (running = false);

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return running;
}

HWND Window::getHWND() const { return hwnd; }
HDC Window::getHDC() const { return hdc; }

int Window::getWidth() const { return width; }
int Window::getHeight() const { return height; }

void Window::setSize(int w, int h)
{
    width = w;
    height = h;
}
