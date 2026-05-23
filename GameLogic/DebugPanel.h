#pragma once
// DebugPanel.h  — Build 0.0.19
// A floating Win32 debug panel: spawn-point sliders + suicide button.
// Usage:
//   1. Call DebugPanel_Init(hInstance) once before the game loop.
//   2. Call DebugPanel_Tick()          each frame (non-blocking).
//   3. Call DebugPanel_PollCommands(&player, SPAWN_X, SPAWN_Y) each frame.
//   4. Call DebugPanel_Destroy()       on shutdown.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// commctrl.h needs _WIN32_IE defined before windows.h under MinGW
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <windows.h>
#include <commctrl.h>   // trackbar / slider — must come after windows.h

// ---- Forward-declare the game types we need ----
struct Player;          // defined in Player.h

// ---- Control IDs ----
#define DBG_IDC_SLIDER_X      1001
#define DBG_IDC_SLIDER_Y      1002
#define DBG_IDC_LABEL_X       1003
#define DBG_IDC_LABEL_Y       1004
#define DBG_IDC_BTN_SUICIDE   1005
#define DBG_IDC_BTN_TELEPORT  1006
#define DBG_IDC_STATIC_X      1007
#define DBG_IDC_STATIC_Y      1008
#define DBG_IDC_BTN_MAXHP     1009
#define DBG_IDC_BTN_FULLAMMO  1010
#define DBG_IDC_STATIC_HP     1011
#define DBG_IDC_STATIC_AMMO   1012

// ---- World-space slider range ----
#define DBG_WORLD_MIN_X   -2000
#define DBG_WORLD_MAX_X    4000
#define DBG_WORLD_MIN_Y   -500
#define DBG_WORLD_MAX_Y    1000

// ---- Commands the panel can issue ----
enum class DebugCmd : int
{
    NONE        = 0,
    SUICIDE     = 1,
    TELEPORT    = 2,
    MAX_HP      = 3,
    FULL_AMMO   = 4,
};

// ---- Shared state (written by the Win32 thread, read by the game thread) ----
struct DebugPanelState
{
    volatile LONG pendingCmd;   // DebugCmd, interlocked
    volatile LONG spawnX;       // world units
    volatile LONG spawnY;       // world units
};

// ---- Public API ----
bool  DebugPanel_Init(HINSTANCE hInstance);
void  DebugPanel_Tick();                    // pump panel messages (call each game frame)
void  DebugPanel_UpdateStatus(int hp, int maxHp, int ammo, int maxAmmo);
void  DebugPanel_PollCommands(Player* player,   // may be nullptr if dead
                               float& outSpawnX,
                               float& outSpawnY,
                               bool&  isDead,
                               int    maxAmmo);
void  DebugPanel_Destroy();

// ---- Internal (implementation detail, not for callers) ----
extern DebugPanelState g_dbgState;
