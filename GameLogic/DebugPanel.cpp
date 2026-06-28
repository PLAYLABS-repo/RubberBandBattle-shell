// DebugPanel.cpp  — Build 0.0.19
// Floating Win32 debug window: spawn-point sliders + suicide / utility buttons.
// MinGW: link with -lcomctl32

#include "DebugPanel.h"
#include "Player.h"       // for Player struct
#include "Constants.h"    // for MAX_AMMO, PUNCH_COOLDOWN …

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Shared state — zero-initialised
// ---------------------------------------------------------------------------
DebugPanelState g_dbgState = { (LONG)DebugCmd::NONE, 200, 436 };

// ---------------------------------------------------------------------------
// Private window state
// ---------------------------------------------------------------------------
static HWND  s_hwnd        = nullptr;
static HWND  s_sliderX     = nullptr;
static HWND  s_sliderY     = nullptr;
static HWND  s_labelX      = nullptr;
static HWND  s_labelY      = nullptr;
static HWND  s_statusHp    = nullptr;
static HWND  s_statusAmmo  = nullptr;
static HFONT s_hFont       = nullptr;
static HFONT s_hFontBold   = nullptr;
static HBRUSH s_hBrushBg   = nullptr;
static HBRUSH s_hBrushSection = nullptr;

// Colour scheme — dark developer aesthetic
static const COLORREF COL_BG        = RGB(18,  20,  28);
static const COLORREF COL_SECTION   = RGB(28,  32,  44);
static const COLORREF COL_TEXT      = RGB(200, 210, 230);
static const COLORREF COL_ACCENT    = RGB(80,  180, 255);
static const COLORREF COL_DANGER    = RGB(255,  70,  70);
static const COLORREF COL_SUCCESS   = RGB(60,  220, 130);
static const COLORREF COL_WARN      = RGB(255, 190,  50);

// ---------------------------------------------------------------------------
// Helper: create a label (static control)
// ---------------------------------------------------------------------------
static HWND MakeLabel(HWND parent, const char* text, int x, int y, int w, int h, int id, HFONT font)
{
    HWND hw = CreateWindowExA(0, "STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, parent, (HMENU)(UINT_PTR)id, nullptr, nullptr);
    if (font) SendMessage(hw, WM_SETFONT, (WPARAM)font, TRUE);
    return hw;
}

// ---------------------------------------------------------------------------
// Helper: create a button
// ---------------------------------------------------------------------------
static HWND MakeButton(HWND parent, const char* text, int x, int y, int w, int h, int id, HFONT font)
{
    HWND hw = CreateWindowExA(0, "BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, parent, (HMENU)(UINT_PTR)id, nullptr, nullptr);
    if (font) SendMessage(hw, WM_SETFONT, (WPARAM)font, TRUE);
    return hw;
}

// ---------------------------------------------------------------------------
// Helper: create a trackbar slider
// ---------------------------------------------------------------------------
static HWND MakeSlider(HWND parent, int x, int y, int w, int h, int id,
                        int minVal, int maxVal, int initVal)
{
    HWND hw = CreateWindowExA(0, TRACKBAR_CLASSA, nullptr,
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
        x, y, w, h, parent, (HMENU)(UINT_PTR)id, nullptr, nullptr);
    SendMessage(hw, TBM_SETRANGE,    TRUE, MAKELPARAM(minVal, maxVal));
    SendMessage(hw, TBM_SETPOS,      TRUE, (LPARAM)initVal);
    SendMessage(hw, TBM_SETPAGESIZE, 0,    10);
    return hw;
}

// ---------------------------------------------------------------------------
// Update the X / Y value labels from the sliders
// ---------------------------------------------------------------------------
static void RefreshSliderLabels()
{
    int vx = (int)SendMessage(s_sliderX, TBM_GETPOS, 0, 0);
    int vy = (int)SendMessage(s_sliderY, TBM_GETPOS, 0, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "X : %d", vx);
    SetWindowTextA(s_labelX, buf);
    snprintf(buf, sizeof(buf), "Y : %d", vy);
    SetWindowTextA(s_labelY, buf);

    // Write to shared state
    InterlockedExchange(&g_dbgState.spawnX, (LONG)vx);
    InterlockedExchange(&g_dbgState.spawnY, (LONG)vy);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK DebugWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    // --- Erase background with our custom colour ---
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wp;
        RECT rc; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, s_hBrushBg);
        return 1;
    }

    // --- Paint section dividers & titles ---
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        SetBkColor(hdc, COL_BG);
        SetTextColor(hdc, COL_ACCENT);
        SelectObject(hdc, s_hFontBold);

        // Section: Spawn Point
        RECT rc1 = { 8, 8, 284, 26 };
        FillRect(hdc, &rc1, s_hBrushSection);
        DrawTextA(hdc, "  SPAWN POINT", -1, &rc1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Section: Player Tools
        RECT rc2 = { 8, 200, 284, 218 };
        FillRect(hdc, &rc2, s_hBrushSection);
        DrawTextA(hdc, "  PLAYER TOOLS", -1, &rc2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Section: Status
        RECT rc3 = { 8, 310, 284, 328 };
        FillRect(hdc, &rc3, s_hBrushSection);
        DrawTextA(hdc, "  STATUS", -1, &rc3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Hairline separators
        HPEN pen = CreatePen(PS_SOLID, 1, COL_ACCENT);
        HPEN old = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 8, 26, nullptr); LineTo(hdc, 284, 26);
        MoveToEx(hdc, 8, 218, nullptr); LineTo(hdc, 284, 218);
        MoveToEx(hdc, 8, 328, nullptr); LineTo(hdc, 284, 328);
        SelectObject(hdc, old);
        DeleteObject(pen);

        EndPaint(hwnd, &ps);
        return 0;
    }

    // --- Colour child controls ---
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, COL_TEXT);
        SetBkColor(hdc, COL_BG);
        return (LRESULT)s_hBrushBg;
    }

    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, COL_TEXT);
        SetBkColor(hdc, COL_SECTION);
        return (LRESULT)s_hBrushSection;
    }

    // --- Slider moved ---
    case WM_HSCROLL:
    {
        HWND slider = (HWND)lp;
        if (slider == s_sliderX || slider == s_sliderY)
            RefreshSliderLabels();
        return 0;
    }

    // --- Buttons ---
    case WM_COMMAND:
    {
        WORD id = LOWORD(wp);
        switch (id)
        {
        case DBG_IDC_BTN_SUICIDE:
            InterlockedExchange(&g_dbgState.pendingCmd, (LONG)DebugCmd::SUICIDE);
            break;
        case DBG_IDC_BTN_TELEPORT:
            InterlockedExchange(&g_dbgState.pendingCmd, (LONG)DebugCmd::TELEPORT);
            break;
        case DBG_IDC_BTN_MAXHP:
            InterlockedExchange(&g_dbgState.pendingCmd, (LONG)DebugCmd::MAX_HP);
            break;
        case DBG_IDC_BTN_FULLAMMO:
            InterlockedExchange(&g_dbgState.pendingCmd, (LONG)DebugCmd::FULL_AMMO);
            break;
        }
        return 0;
    }

    // Prevent closing — just hide
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
}

// ---------------------------------------------------------------------------
// DebugPanel_Init
// ---------------------------------------------------------------------------
bool DebugPanel_Init(HINSTANCE hInstance)
{
    // Init common controls (needed for trackbar)
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    // Brushes
    s_hBrushBg      = CreateSolidBrush(COL_BG);
    s_hBrushSection = CreateSolidBrush(COL_SECTION);

    // Fonts
    s_hFont = CreateFontA(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "TUBAnComic Bold");

    s_hFontBold = CreateFontA(
        14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "TUBAnComic Bold");

    // Register window class
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = DebugWndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = s_hBrushBg;
    wc.lpszClassName = "Debug Panel";
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    if (!RegisterClassExA(&wc)) return false;

    // Panel window — 292 × 410, top-right corner
    s_hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "Debug Panel", "Debug Panel",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        10, 10, 300, 420,
        nullptr, nullptr, hInstance, nullptr);

    if (!s_hwnd) return false;

    // ---- SECTION: SPAWN POINT ----------------------------------------
    // X slider
    MakeLabel(s_hwnd, "Spawn X", 12, 32, 80, 16, DBG_IDC_STATIC_X, s_hFont);
    s_labelX = MakeLabel(s_hwnd, "X : 200", 96, 32, 100, 16, DBG_IDC_LABEL_X, s_hFontBold);
    s_sliderX = MakeSlider(s_hwnd, 12, 52, 270, 28, DBG_IDC_SLIDER_X,
                            DBG_WORLD_MIN_X, DBG_WORLD_MAX_X, 200);

    // Y slider
    MakeLabel(s_hwnd, "Spawn Y", 12, 88, 80, 16, DBG_IDC_STATIC_Y, s_hFont);
    s_labelY = MakeLabel(s_hwnd, "Y : 436", 96, 88, 100, 16, DBG_IDC_LABEL_Y, s_hFontBold);
    s_sliderY = MakeSlider(s_hwnd, 12, 108, 270, 28, DBG_IDC_SLIDER_Y,
                            DBG_WORLD_MIN_Y, DBG_WORLD_MAX_Y, 436);

    // Preview note
    MakeLabel(s_hwnd, "Going to the desired position", 12, 144, 270, 16, 0, s_hFont);
    MakeLabel(s_hwnd, "", 12, 162, 270, 16, 0, s_hFont);

    // Teleport button
    MakeButton(s_hwnd, "Go now", 12, 184, 130, 26, DBG_IDC_BTN_TELEPORT, s_hFontBold);

    // ---- SECTION: PLAYER TOOLS ---------------------------------------
    // Suicide button (wide, red-ish)
    HWND btnSuicide = MakeButton(s_hwnd, "KILL USER", 12, 224, 270, 34,
                                  DBG_IDC_BTN_SUICIDE, s_hFontBold);
    (void)btnSuicide;

    // Max HP + Full Ammo
    MakeButton(s_hwnd, "Eternal life",   12, 266, 130, 28, DBG_IDC_BTN_MAXHP,    s_hFont);
    MakeButton(s_hwnd, "Fill magazine",        150, 266, 130, 28, DBG_IDC_BTN_FULLAMMO, s_hFont);

    // ---- SECTION: STATUS ---------------------------------------------
    s_statusHp   = MakeLabel(s_hwnd, "HP   : ---",   12, 334, 270, 16, DBG_IDC_STATIC_HP,   s_hFont);
    s_statusAmmo = MakeLabel(s_hwnd, "AMMO : ---",   12, 356, 270, 16, DBG_IDC_STATIC_AMMO, s_hFont);
    MakeLabel(s_hwnd, "Build 0.0.19  |  RubberBandBattle", 12, 390, 270, 14, 0, s_hFont);

    // Initialise shared spawn position
    InterlockedExchange(&g_dbgState.spawnX, 200);
    InterlockedExchange(&g_dbgState.spawnY, 436);

    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);
    return true;
}

// ---------------------------------------------------------------------------
// DebugPanel_Tick — call each game frame (pumps panel messages non-blocking)
// ---------------------------------------------------------------------------
void DebugPanel_Tick()
{
    if (!s_hwnd) return;
    MSG msg;
    while (PeekMessageA(&msg, s_hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

// ---------------------------------------------------------------------------
// DebugPanel_UpdateStatus — push current hp/ammo into the status labels
// ---------------------------------------------------------------------------
void DebugPanel_UpdateStatus(int hp, int maxHp, int ammo, int maxAmmo)
{
    if (!s_statusHp || !s_statusAmmo) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "HEALTH   : %d / %d", hp, maxHp);
    SetWindowTextA(s_statusHp, buf);
    snprintf(buf, sizeof(buf), "BULLET : %d / %d", ammo, maxAmmo);
    SetWindowTextA(s_statusAmmo, buf);
}

// ---------------------------------------------------------------------------
// DebugPanel_PollCommands — apply any pending debug command to the game state
// Call AFTER game logic each frame.
// ---------------------------------------------------------------------------
void DebugPanel_PollCommands(Player* player,
                              float& outSpawnX,
                              float& outSpawnY,
                              bool&  isDead,
                              int    maxAmmo)
{
    // Always update spawn XY from sliders
    outSpawnX = (float)InterlockedCompareExchange(&g_dbgState.spawnX, 0, 0);
    outSpawnY = (float)InterlockedCompareExchange(&g_dbgState.spawnY, 0, 0);

    // Consume pending command (swap to NONE)
    LONG cmd = InterlockedExchange(&g_dbgState.pendingCmd, (LONG)DebugCmd::NONE);

    switch ((DebugCmd)cmd)
    {
    case DebugCmd::SUICIDE:
        if (player && !isDead)
            player->hp = 0;   // death check in main loop handles the rest
        break;

    case DebugCmd::TELEPORT:
        if (player && !isDead)
        {
            player->x    = outSpawnX;
            player->y    = outSpawnY;
            player->baseY = outSpawnY;
            player->sprite->position       = { player->x, player->y };
            player->sprite->targetPosition = { player->x, player->y };
        }
        break;

    case DebugCmd::MAX_HP:
        if (player)
            player->hp = player->maxHp;
        break;

    case DebugCmd::FULL_AMMO:
        if (player)
            player->ammo = maxAmmo;
        break;

    default: break;
    }
}

// ---------------------------------------------------------------------------
// DebugPanel_Destroy
// ---------------------------------------------------------------------------
void DebugPanel_Destroy()
{
    if (s_hwnd)   { DestroyWindow(s_hwnd); s_hwnd = nullptr; }
    if (s_hFont)       { DeleteObject(s_hFont);       s_hFont = nullptr; }
    if (s_hFontBold)   { DeleteObject(s_hFontBold);   s_hFontBold = nullptr; }
    if (s_hBrushBg)    { DeleteObject(s_hBrushBg);    s_hBrushBg = nullptr; }
    if (s_hBrushSection){ DeleteObject(s_hBrushSection); s_hBrushSection = nullptr; }
}
