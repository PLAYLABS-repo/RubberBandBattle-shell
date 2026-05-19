#pragma once
// =============================================================
// PlaylabsUI — Immediate-mode UI layer for PlaylabsGL
// OpenGL 1.1 compatible — no shaders, no VAOs, no VBOs.
//
// Usage:
//   #include "PlaylabsUI.h"
//
//   // Inside your game loop:
//   UI::BeginFrame(screenW, screenH);
//
//   if (UI::Button("Play", 100, 200, 160, 48))
//       Event("ui_clicked", nullptr, (void*)"Play");
//
//   UI::Label("Score: 99", 20, 20);
//   UI::Image(myImage, 300, 100, 64, 64);
//
//   UI::EndFrame();
// =============================================================

#include <GL/gl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <cmath>

#include "PlaylabsGL.h"   // EventBus, EventData, Image, MousePos, KeyDown …

// ─────────────────────────────────────────────────────────────
// Forward declarations (defined at the bottom of this file)
// ─────────────────────────────────────────────────────────────
namespace UI { struct Style; }

// =============================================================
// UIRect  — axis-aligned rectangle helper (local, no AABB dep.)
// =============================================================
struct UIRect
{
    float x, y, w, h;
    bool contains(float px, float py) const
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

// =============================================================
// NineSlice — 9-slice scaling support for images
// =============================================================
struct NineSlice
{
    float left, right, top, bottom;  // border sizes (in pixels)
    bool  enabled = false;

    NineSlice() : left(0), right(0), top(0), bottom(0), enabled(false) {}
    NineSlice(float l, float r, float t, float b)
        : left(l), right(r), top(t), bottom(b), enabled(true) {}

    // Draw a 9-slice scaled image
    void draw(::Image* img, float dstX, float dstY, float dstW, float dstH,
              float tintR = 1, float tintG = 1, float tintB = 1, float tintA = 1) const
    {
        if (!img || !img->textureID) return;

        float srcX = 0, srcY = 0;
        float srcW = (float)img->width;
        float srcH = (float)img->height;

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, img->textureID);
        glColor4f(tintR, tintG, tintB, tintA);

        // Calculate source borders
        float srcLeft = srcX + left;
        float srcRight = srcX + srcW - right;
        float srcTop = srcY + top;
        float srcBottom = srcY + srcH - bottom;

        // Calculate destination borders
        float dstLeft = dstX + left;
        float dstRight = dstX + dstW - right;
        float dstTop = dstY + top;
        float dstBottom = dstY + dstH - bottom;

        // 9 sections
        // Section 0: Top-left
        _draw9SliceSection(img, srcX, srcY, left, top, dstX, dstY, left, top);
        // Section 1: Top-middle
        _draw9SliceSection(img, srcLeft, srcY, srcW - left - right, top,
                          dstLeft, dstY, dstW - left - right, top);
        // Section 2: Top-right
        _draw9SliceSection(img, srcRight, srcY, right, top,
                          dstRight, dstY, right, top);
        // Section 3: Middle-left
        _draw9SliceSection(img, srcX, srcTop, left, srcH - top - bottom,
                          dstX, dstTop, left, dstH - top - bottom);
        // Section 4: Middle-middle (center)
        _draw9SliceSection(img, srcLeft, srcTop, srcW - left - right, srcH - top - bottom,
                          dstLeft, dstTop, dstW - left - right, dstH - top - bottom);
        // Section 5: Middle-right
        _draw9SliceSection(img, srcRight, srcTop, right, srcH - top - bottom,
                          dstRight, dstTop, right, dstH - top - bottom);
        // Section 6: Bottom-left
        _draw9SliceSection(img, srcX, srcBottom, left, bottom,
                          dstX, dstBottom, left, bottom);
        // Section 7: Bottom-middle
        _draw9SliceSection(img, srcLeft, srcBottom, srcW - left - right, bottom,
                          dstLeft, dstBottom, dstW - left - right, bottom);
        // Section 8: Bottom-right
        _draw9SliceSection(img, srcRight, srcBottom, right, bottom,
                          dstRight, dstBottom, right, bottom);
    }

private:
    void _draw9SliceSection(::Image* img,
                           float srcX, float srcY, float srcW, float srcH,
                           float dstX, float dstY, float dstW, float dstH) const
    {
        if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;

        float texW = (float)img->width;
        float texH = (float)img->height;

        float u0 = srcX / texW;
        float v0 = srcY / texH;
        float u1 = (srcX + srcW) / texW;
        float v1 = (srcY + srcH) / texH;

        glBegin(GL_QUADS);
            glTexCoord2f(u0, v0); glVertex2f(dstX, dstY);
            glTexCoord2f(u1, v0); glVertex2f(dstX + dstW, dstY);
            glTexCoord2f(u1, v1); glVertex2f(dstX + dstW, dstY + dstH);
            glTexCoord2f(u0, v1); glVertex2f(dstX, dstY + dstH);
        glEnd();
    }
};

// =============================================================
// UIClickable — wraps a named region and fires EventBus events
//
//   isClickable(true,  "my_button")   — register / activate
//   isClickable(false, "my_button")   — deactivate
//
// While active the region listens for the internal
// "ui_region_test" pulse emitted by UI::EndFrame() and fires:
//   "ui_hover"   when the mouse is inside
//   "ui_press"   on mouse-button-down inside
//   "ui_release" on mouse-button-up  inside
//   "ui_click"   on release after a press (full click)
// =============================================================
struct UIClickable
{
    std::string name;
    UIRect      rect   = {0,0,0,0};
    bool        active = false;

    // runtime state
    bool _wasPressed = false;

    // subscription handle so we can unsubscribe cleanly
    int  _subHandle  = -1;

    // ── Activate / deactivate ────────────────────────────────
    void isClickable(bool enable, const std::string& uiName)
    {
        if (enable == active && uiName == name) return;

        // Deactivate old subscription first
        if (_subHandle != -1)
        {
            EventBus::Unsubscribe("_ui_poll", _subHandle);
            _subHandle = -1;
        }

        name   = uiName;
        active = enable;
        _wasPressed = false;

        if (!enable) return;

        // Subscribe to the internal poll event emitted each frame
        UIClickable* self = this;
        _subHandle = EventBus::Subscribe("_ui_poll",
            [self](const EventData& ed)
            {
                if (!self->active) return;

                int mx = 0, my = 0;
                MousePos(&mx, &my);
                float fx = (float)mx, fy = (float)my;

                bool inside = self->rect.contains(fx, fy);

                // left mouse button: VK_LBUTTON == 0x01
                bool lmbDown = (KeyDown(0x01) != 0);

                if (inside)
                {
                    // hover
                    EventBus::Emit("ui_hover",
                        (void*)self->name.c_str(), nullptr);

                    if (lmbDown && !self->_wasPressed)
                    {
                        self->_wasPressed = true;
                        EventBus::Emit("ui_press",
                            (void*)self->name.c_str(), nullptr);
                    }
                    else if (!lmbDown && self->_wasPressed)
                    {
                        self->_wasPressed = false;
                        EventBus::Emit("ui_release",
                            (void*)self->name.c_str(), nullptr);
                        EventBus::Emit("ui_click",
                            (void*)self->name.c_str(), nullptr);
                    }
                }
                else
                {
                    if (!lmbDown) self->_wasPressed = false;
                }
            }
        );
    }

    // Convenience: update the hit-rect every frame
    void setRect(float x, float y, float w, float h)
    { rect = {x, y, w, h}; }

    ~UIClickable() { isClickable(false, name); }
};

// =============================================================
// UI namespace — immediate-mode draw + widget helpers
// =============================================================
namespace UI
{
    // ── Internal state ────────────────────────────────────────
    // Stored inside a function-local static so there is exactly one
    // instance across all translation units — compatible with C++11/GCC 6.
    namespace _detail
    {
        struct State
        {
            int         s_screenW    = 800;
            int         s_screenH    = 600;
            std::size_t s_hotId      = 0;
            std::size_t s_activeId   = 0;
            bool        s_lmbDown    = false;
            bool        s_lmbDownPrev= false;
            int         s_mouseX     = 0;
            int         s_mouseY     = 0;
        };

        inline State& get()
        {
            static State inst;
            return inst;
        }

        // Convenience references — just use _detail::get().fieldName directly
        // in the implementation below.

        inline std::size_t hashStr(const std::string& s)
        {
            // FNV-1a 32-bit
            std::size_t h = 2166136261u;
            for (unsigned char c : s)
                h = (h ^ c) * 16777619u;
            return h;
        }
    }

    // ── Style ─────────────────────────────────────────────────
    struct Style
    {
        // Normal state
        float bgR   = 0.18f, bgG   = 0.18f, bgB   = 0.22f, bgA   = 1.0f;
        // Hover state
        float hovR  = 0.26f, hovG  = 0.26f, hovB  = 0.34f, hovA  = 1.0f;
        // Active (pressed) state
        float actR  = 0.10f, actG  = 0.10f, actB  = 0.14f, actA  = 1.0f;
        // Text / label
        float txtR  = 1.0f,  txtG  = 1.0f,  txtB  = 1.0f,  txtA  = 1.0f;
        // Border
        float borR  = 0.50f, borG  = 0.50f, borB  = 0.60f, borA  = 1.0f;
        float borderWidth = 1.5f;
        // Corner rounding (0 = sharp)
        float cornerRadius = 5.0f;
        // Padding inside buttons
        float padX = 12.0f, padY = 6.0f;
    };

    inline Style& getStyle()
    {
        static Style inst;
        return inst;
    }

    // ── Low-level GL 1.1 draw helpers ─────────────────────────

    // Set up an orthographic 2-D projection (pixel-perfect, Y-down)
    inline void _setupOrtho(int w, int h)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, (double)w, (double)h, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    inline void _restoreMatrices()
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    inline void _drawFilledRect(float x, float y, float w, float h,
                                float r, float g, float b, float a)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
            glVertex2f(x,     y);
            glVertex2f(x + w, y);
            glVertex2f(x + w, y + h);
            glVertex2f(x,     y + h);
        glEnd();
    }

    inline void _drawBorderRect(float x, float y, float w, float h,
                                float r, float g, float b, float a,
                                float lw)
    {
        glColor4f(r, g, b, a);
        glLineWidth(lw);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x,     y);
            glVertex2f(x + w, y);
            glVertex2f(x + w, y + h);
            glVertex2f(x,     y + h);
        glEnd();
        glLineWidth(1.0f);
    }

    // Approximate rounded rect via triangle fan (GL 1.1 compatible)
    inline void _drawRoundedRect(float x, float y, float w, float h,
                                 float rad,
                                 float r, float g, float b, float a,
                                 bool filled)
    {
        if (rad <= 0.0f)
        {
            if (filled) _drawFilledRect(x, y, w, h, r, g, b, a);
            else        _drawBorderRect(x, y, w, h, r, g, b, a, 1.5f);
            return;
        }

        const int   SEG   = 8;
        const float PI    = 3.14159265f;
        const float clamp = std::fmin(rad, std::fmin(w, h) * 0.5f);

        // Corner arc centres and start angles
        float startAngle[4] = { PI,       PI * 1.5f, 0.0f,    PI * 0.5f };
        float ccx[4] = { x + clamp,     x + w - clamp,
                          x + w - clamp, x + clamp      };
        float ccy[4] = { y + clamp,     y + clamp,
                          y + h - clamp, y + h - clamp  };

        glColor4f(r, g, b, a);
        if (filled)
            glBegin(GL_TRIANGLE_FAN);
        else
        {
            glLineWidth(1.5f);
            glBegin(GL_LINE_LOOP);
        }

        for (int c = 0; c < 4; ++c)
        {
            float baseAngle = startAngle[c];
            for (int s = 0; s <= SEG; ++s)
            {
                float angle = baseAngle + (PI * 0.5f) * ((float)s / SEG);
                float vx = ccx[c] + std::cos(angle) * clamp;
                float vy = ccy[c] + std::sin(angle) * clamp;
                glVertex2f(vx, vy);
            }
        }
        glEnd();
        if (!filled) glLineWidth(1.0f);
    }

    // Simple bitmap-style dot-matrix character (5×7 pixels)
    // Only renders ASCII 32-126. Each char is a column bitmask array.
    namespace _font
    {
        // 5-column × 7-row bitmask font (bit 6 = top row)
        static const unsigned char glyphs[][5] = {
            // 32 SPACE
            {0x00,0x00,0x00,0x00,0x00},
            // 33 !
            {0x00,0x5F,0x00,0x00,0x00},
            // 34 "
            {0x07,0x00,0x07,0x00,0x00},
            // 35 #
            {0x14,0x7F,0x14,0x7F,0x14},
            // 36 $
            {0x24,0x2A,0x7F,0x2A,0x12},
            // 37 %
            {0x23,0x13,0x08,0x64,0x62},
            // 38 &
            {0x36,0x49,0x55,0x22,0x50},
            // 39 '
            {0x00,0x05,0x03,0x00,0x00},
            // 40 (
            {0x00,0x1C,0x22,0x41,0x00},
            // 41 )
            {0x00,0x41,0x22,0x1C,0x00},
            // 42 *
            {0x08,0x2A,0x1C,0x2A,0x08},
            // 43 +
            {0x08,0x08,0x3E,0x08,0x08},
            // 44 ,
            {0x00,0x50,0x30,0x00,0x00},
            // 45 -
            {0x08,0x08,0x08,0x08,0x08},
            // 46 .
            {0x00,0x60,0x60,0x00,0x00},
            // 47 /
            {0x20,0x10,0x08,0x04,0x02},
            // 48 0
            {0x3E,0x51,0x49,0x45,0x3E},
            // 49 1
            {0x00,0x42,0x7F,0x40,0x00},
            // 50 2
            {0x42,0x61,0x51,0x49,0x46},
            // 51 3
            {0x21,0x41,0x45,0x4B,0x31},
            // 52 4
            {0x18,0x14,0x12,0x7F,0x10},
            // 53 5
            {0x27,0x45,0x45,0x45,0x39},
            // 54 6
            {0x3C,0x4A,0x49,0x49,0x30},
            // 55 7
            {0x01,0x71,0x09,0x05,0x03},
            // 56 8
            {0x36,0x49,0x49,0x49,0x36},
            // 57 9
            {0x06,0x49,0x49,0x29,0x1E},
            // 58 :
            {0x00,0x36,0x36,0x00,0x00},
            // 59 ;
            {0x00,0x56,0x36,0x00,0x00},
            // 60 <
            {0x08,0x14,0x22,0x41,0x00},
            // 61 =
            {0x14,0x14,0x14,0x14,0x14},
            // 62 >
            {0x00,0x41,0x22,0x14,0x08},
            // 63 ?
            {0x02,0x01,0x51,0x09,0x06},
            // 64 @
            {0x32,0x49,0x79,0x41,0x3E},
            // 65 A
            {0x7E,0x11,0x11,0x11,0x7E},
            // 66 B
            {0x7F,0x49,0x49,0x49,0x36},
            // 67 C
            {0x3E,0x41,0x41,0x41,0x22},
            // 68 D
            {0x7F,0x41,0x41,0x22,0x1C},
            // 69 E
            {0x7F,0x49,0x49,0x49,0x41},
            // 70 F
            {0x7F,0x09,0x09,0x09,0x01},
            // 71 G
            {0x3E,0x41,0x49,0x49,0x7A},
            // 72 H
            {0x7F,0x08,0x08,0x08,0x7F},
            // 73 I
            {0x00,0x41,0x7F,0x41,0x00},
            // 74 J
            {0x20,0x40,0x41,0x3F,0x01},
            // 75 K
            {0x7F,0x08,0x14,0x22,0x41},
            // 76 L
            {0x7F,0x40,0x40,0x40,0x40},
            // 77 M
            {0x7F,0x02,0x0C,0x02,0x7F},
            // 78 N
            {0x7F,0x04,0x08,0x10,0x7F},
            // 79 O
            {0x3E,0x41,0x41,0x41,0x3E},
            // 80 P
            {0x7F,0x09,0x09,0x09,0x06},
            // 81 Q
            {0x3E,0x41,0x51,0x21,0x5E},
            // 82 R
            {0x7F,0x09,0x19,0x29,0x46},
            // 83 S
            {0x46,0x49,0x49,0x49,0x31},
            // 84 T
            {0x01,0x01,0x7F,0x01,0x01},
            // 85 U
            {0x3F,0x40,0x40,0x40,0x3F},
            // 86 V
            {0x1F,0x20,0x40,0x20,0x1F},
            // 87 W
            {0x3F,0x40,0x38,0x40,0x3F},
            // 88 X
            {0x63,0x14,0x08,0x14,0x63},
            // 89 Y
            {0x07,0x08,0x70,0x08,0x07},
            // 90 Z
            {0x61,0x51,0x49,0x45,0x43},
            // 91 [
            {0x00,0x7F,0x41,0x41,0x00},
            // 92 backslash
            {0x02,0x04,0x08,0x10,0x20},
            // 93 ]
            {0x00,0x41,0x41,0x7F,0x00},
            // 94 ^
            {0x04,0x02,0x01,0x02,0x04},
            // 95 _
            {0x40,0x40,0x40,0x40,0x40},
            // 96 `
            {0x00,0x01,0x02,0x04,0x00},
            // 97 a
            {0x20,0x54,0x54,0x54,0x78},
            // 98 b
            {0x7F,0x48,0x44,0x44,0x38},
            // 99 c
            {0x38,0x44,0x44,0x44,0x20},
            // 100 d
            {0x38,0x44,0x44,0x48,0x7F},
            // 101 e
            {0x38,0x54,0x54,0x54,0x18},
            // 102 f
            {0x08,0x7E,0x09,0x01,0x02},
            // 103 g
            {0x0C,0x52,0x52,0x52,0x3E},
            // 104 h
            {0x7F,0x08,0x04,0x04,0x78},
            // 105 i
            {0x00,0x44,0x7D,0x40,0x00},
            // 106 j
            {0x20,0x40,0x44,0x3D,0x00},
            // 107 k
            {0x7F,0x10,0x28,0x44,0x00},
            // 108 l
            {0x00,0x41,0x7F,0x40,0x00},
            // 109 m
            {0x7C,0x04,0x18,0x04,0x78},
            // 110 n
            {0x7C,0x08,0x04,0x04,0x78},
            // 111 o
            {0x38,0x44,0x44,0x44,0x38},
            // 112 p
            {0x7C,0x14,0x14,0x14,0x08},
            // 113 q
            {0x08,0x14,0x14,0x18,0x7C},
            // 114 r
            {0x7C,0x08,0x04,0x04,0x08},
            // 115 s
            {0x48,0x54,0x54,0x54,0x20},
            // 116 t
            {0x04,0x3F,0x44,0x40,0x20},
            // 117 u
            {0x3C,0x40,0x40,0x40,0x3C},
            // 118 v
            {0x1C,0x20,0x40,0x20,0x1C},
            // 119 w
            {0x3C,0x40,0x30,0x40,0x3C},
            // 120 x
            {0x44,0x28,0x10,0x28,0x44},
            // 121 y
            {0x0C,0x50,0x50,0x50,0x3C},
            // 122 z
            {0x44,0x64,0x54,0x4C,0x44},
            // 123 {
            {0x00,0x08,0x36,0x41,0x00},
            // 124 |
            {0x00,0x00,0x7F,0x00,0x00},
            // 125 }
            {0x00,0x41,0x36,0x08,0x00},
            // 126 ~
            {0x08,0x04,0x08,0x10,0x08},
        };

        // Draw a single character using GL_POINTS (pixel-perfect)
        inline void drawChar(char ch, float x, float y, float scale,
                             float r, float g, float b, float a)
        {
            int idx = (unsigned char)ch - 32;
            if (idx < 0 || idx >= 95) idx = 0;
            const unsigned char* col = glyphs[idx];

            glColor4f(r, g, b, a);
            glBegin(GL_QUADS);
            for (int cx = 0; cx < 5; ++cx)
            {
                for (int row = 0; row < 7; ++row)
                {
                    if (col[cx] & (1 << (6 - row)))
                    {
                        float px = x + cx * scale;
                        float py = y + row * scale;
                        glVertex2f(px,          py);
                        glVertex2f(px + scale,  py);
                        glVertex2f(px + scale,  py + scale);
                        glVertex2f(px,          py + scale);
                    }
                }
            }
            glEnd();
        }

        inline float textWidth(const std::string& s, float scale)
        { return s.size() * 6.0f * scale; }

        inline float textHeight(float scale)
        { return 7.0f * scale; }

        inline void drawText(const std::string& text, float x, float y,
                             float scale,
                             float r, float g, float b, float a)
        {
            float cx = x;
            for (char c : text)
            {
                drawChar(c, cx, y, scale, r, g, b, a);
                cx += 6.0f * scale;
            }
        }
    } // namespace _font

    // ── Public API ────────────────────────────────────────────

    // Call once per frame BEFORE drawing UI
    inline void BeginFrame(int screenW, int screenH)
    {
        _detail::State& S = _detail::get();
        S.s_screenW      = screenW;
        S.s_screenH      = screenH;
        S.s_lmbDownPrev  = S.s_lmbDown;
        S.s_lmbDown      = (KeyDown(0x01) != 0);
        MousePos(&S.s_mouseX, &S.s_mouseY);
        S.s_hotId        = 0;

        // Save all GL state we might disturb
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);

        _setupOrtho(screenW, screenH);
    }

    // Call once per frame AFTER drawing UI
    inline void EndFrame()
    {
        // Emit internal poll so all UIClickables get updated
        EventBus::Emit("_ui_poll", nullptr, nullptr);

        _restoreMatrices();
        glPopAttrib();

        // If mouse released with no widget active, clear activeId
        if (!_detail::get().s_lmbDown) _detail::get().s_activeId = 0;
    }

    // ── Label ────────────────────────────────────────────────
    // Draws text at (x, y). scale = pixel size multiplier.
    inline void Label(const std::string& text,
                      float x, float y,
                      float scale = 2.0f,
                      float r = 1, float g = 1, float b = 1, float a = 1)
    {
        glDisable(GL_TEXTURE_2D);
        _font::drawText(text, x, y, scale, r, g, b, a);
        glEnable(GL_TEXTURE_2D);
    }

    // ── Button ───────────────────────────────────────────────
    // Returns true the frame the button is clicked (released).
    inline bool Button(const std::string& label,
                       float x, float y, float w, float h,
                       float textScale,
                       const Style& st)
    {
        _detail::State& S  = _detail::get();
        std::size_t id     = _detail::hashStr(label);

        float mx = (float)S.s_mouseX;
        float my = (float)S.s_mouseY;
        bool inside = (mx >= x && mx <= x+w && my >= y && my <= y+h);

        if (inside) S.s_hotId = id;
        if (S.s_hotId == id && S.s_lmbDown && !S.s_lmbDownPrev)
            S.s_activeId = id;

        // Choose colour
        float r, g, b, a;
        if (S.s_activeId == id && inside)
        { r=st.actR; g=st.actG; b=st.actB; a=st.actA; }
        else if (S.s_hotId == id)
        { r=st.hovR; g=st.hovG; b=st.hovB; a=st.hovA; }
        else
        { r=st.bgR;  g=st.bgG;  b=st.bgB;  a=st.bgA; }

        glDisable(GL_TEXTURE_2D);

        // Fill
        _drawRoundedRect(x, y, w, h, st.cornerRadius, r, g, b, a, true);
        // Border
        _drawRoundedRect(x, y, w, h, st.cornerRadius,
                         st.borR, st.borG, st.borB, st.borA, false);

        // Centre text
        float tw = _font::textWidth(label, textScale);
        float th = _font::textHeight(textScale);
        float tx = x + (w - tw) * 0.5f;
        float ty = y + (h - th) * 0.5f;
        _font::drawText(label, tx, ty, textScale,
                        st.txtR, st.txtG, st.txtB, st.txtA);

        glEnable(GL_TEXTURE_2D);

        // Click = released inside after pressing
        bool clicked = (S.s_activeId == id && inside &&
                        !S.s_lmbDown && S.s_lmbDownPrev);
        if (clicked) S.s_activeId = 0;
        return clicked;
    }

    // Convenience overload — uses the global style
    inline bool Button(const std::string& label,
                       float x, float y, float w, float h,
                       float textScale = 2.0f)
    { return Button(label, x, y, w, h, textScale, getStyle()); }

    // ── Checkbox ─────────────────────────────────────────────
    // Toggles value on click. Returns new value.
    inline bool Checkbox(const std::string& label,
                         bool& value,
                         float x, float y,
                         float size,
                         float textScale,
                         const Style& st)
    {
        _detail::State& S  = _detail::get();
        std::size_t id     = _detail::hashStr("CB_" + label);

        float mx = (float)S.s_mouseX;
        float my = (float)S.s_mouseY;
        bool inside = (mx >= x && mx <= x+size && my >= y && my <= y+size);

        if (inside) S.s_hotId = id;
        if (S.s_hotId == id && S.s_lmbDown && !S.s_lmbDownPrev)
            S.s_activeId = id;

        bool clicked = (S.s_activeId == id && inside &&
                        !S.s_lmbDown && S.s_lmbDownPrev);
        if (clicked) { value = !value; S.s_activeId = 0; }

        glDisable(GL_TEXTURE_2D);

        // Box fill
        float r = inside ? st.hovR : st.bgR;
        float g = inside ? st.hovG : st.bgG;
        float b = inside ? st.hovB : st.bgB;
        _drawFilledRect(x, y, size, size, r, g, b, 1.0f);
        _drawBorderRect(x, y, size, size,
                        st.borR, st.borG, st.borB, 1.0f, st.borderWidth);

        // Tick mark
        if (value)
        {
            float m = size * 0.2f;
            glColor4f(st.txtR, st.txtG, st.txtB, st.txtA);
            glLineWidth(2.5f);
            glBegin(GL_LINE_STRIP);
                glVertex2f(x + m,           y + size * 0.55f);
                glVertex2f(x + size * 0.4f, y + size - m);
                glVertex2f(x + size - m,    y + m);
            glEnd();
            glLineWidth(1.0f);
        }

        // Label text
        float tx = x + size + 6.0f;
        float ty = y + (size - _font::textHeight(textScale)) * 0.5f;
        _font::drawText(label, tx, ty, textScale,
                        st.txtR, st.txtG, st.txtB, st.txtA);

        glEnable(GL_TEXTURE_2D);
        return value;
    }

    // Convenience overload — uses the global style
    inline bool Checkbox(const std::string& label,
                         bool& value,
                         float x, float y,
                         float size = 20.0f,
                         float textScale = 2.0f)
    { return Checkbox(label, value, x, y, size, textScale, getStyle()); }

    // ── Slider ───────────────────────────────────────────────
    // Horizontal slider. value in [minV, maxV]. Returns true while dragging.
    inline bool Slider(const std::string& id_label,
                       float& value, float minV, float maxV,
                       float x, float y, float w,
                       float trackH,
                       const Style& st)
    {
        _detail::State& S  = _detail::get();
        std::size_t id     = _detail::hashStr("SL_" + id_label);

        float knobR  = trackH * 1.8f;
        float trackY = y + knobR - trackH * 0.5f;
        float t      = (value - minV) / (maxV - minV);
        float knobX  = x + t * w;
        float knobCY = y + knobR;

        float mx = (float)S.s_mouseX;
        float my = (float)S.s_mouseY;
        float dx = mx - knobX, dy = my - knobCY;
        bool onKnob  = (dx*dx + dy*dy) <= (knobR * knobR);
        bool onTrack = (mx >= x && mx <= x+w &&
                        my >= trackY && my <= trackY+trackH);

        if (onKnob || onTrack) S.s_hotId = id;
        if (S.s_hotId == id && S.s_lmbDown && !S.s_lmbDownPrev)
            S.s_activeId = id;

        bool dragging = (S.s_activeId == id && S.s_lmbDown);
        if (dragging)
        {
            float nt = (mx - x) / w;
            nt       = nt < 0.0f ? 0.0f : (nt > 1.0f ? 1.0f : nt);
            value    = minV + nt * (maxV - minV);
        }
        if (!S.s_lmbDown && S.s_activeId == id) S.s_activeId = 0;

        glDisable(GL_TEXTURE_2D);

        // Track
        _drawFilledRect(x, trackY, w, trackH,
                        st.bgR*0.6f, st.bgG*0.6f, st.bgB*0.6f, 1.0f);

        // Filled portion
        float ft = (value - minV) / (maxV - minV);
        _drawFilledRect(x, trackY, w * ft, trackH,
                        st.hovR, st.hovG, st.hovB, 1.0f);

        // Knob
        float kr = knobR;
        float kx = x + ft * w;
        const int KN = 16;
        float kCol_r = dragging ? st.actR : (S.s_hotId == id ? st.hovR : st.bgR + 0.1f);
        float kCol_g = dragging ? st.actG : (S.s_hotId == id ? st.hovG : st.bgG + 0.1f);
        float kCol_b = dragging ? st.actB : (S.s_hotId == id ? st.hovB : st.bgB + 0.1f);
        glColor4f(kCol_r, kCol_g, kCol_b, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(kx, knobCY);
            for (int i = 0; i <= KN; ++i)
            {
                float ang = 3.14159265f * 2.0f * i / KN;
                glVertex2f(kx + std::cos(ang)*kr,
                           knobCY + std::sin(ang)*kr);
            }
        glEnd();
        glColor4f(st.borR, st.borG, st.borB, 1.0f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
            for (int i = 0; i <= KN; ++i)
            {
                float ang = 3.14159265f * 2.0f * i / KN;
                glVertex2f(kx + std::cos(ang)*kr,
                           knobCY + std::sin(ang)*kr);
            }
        glEnd();
        glLineWidth(1.0f);

        glEnable(GL_TEXTURE_2D);
        return dragging;
    }

    // Convenience overload — uses the global style
    inline bool Slider(const std::string& id_label,
                       float& value, float minV, float maxV,
                       float x, float y, float w,
                       float trackH = 6.0f)
    { return Slider(id_label, value, minV, maxV, x, y, w, trackH, getStyle()); }

    // ── Panel ────────────────────────────────────────────────
    // Draws a background panel (no interaction).
    inline void Panel(float x, float y, float w, float h,
                      float r = 0.12f, float g = 0.12f, float b = 0.16f,
                      float a = 0.92f,
                      float cornerRadius = 8.0f)
    {
        glDisable(GL_TEXTURE_2D);
        _drawRoundedRect(x, y, w, h, cornerRadius, r, g, b, a, true);
        _drawRoundedRect(x, y, w, h, cornerRadius,
                         0.4f, 0.4f, 0.5f, 1.0f, false);
        glEnable(GL_TEXTURE_2D);
    }

    // ── Image ────────────────────────────────────────────────
    // Draws a PlaylabsGL Image* at screen position (x,y) sized (w,h).
    inline void Image(::Image* img, float x, float y, float w, float h,
                      float tintR = 1, float tintG = 1,
                      float tintB = 1, float tintA = 1)
    {
        if (!img || !img->textureID) return;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, img->textureID);
        glColor4f(tintR, tintG, tintB, tintA);
        glBegin(GL_QUADS);
            glTexCoord2f(0,0); glVertex2f(x,     y);
            glTexCoord2f(1,0); glVertex2f(x + w, y);
            glTexCoord2f(1,1); glVertex2f(x + w, y + h);
            glTexCoord2f(0,1); glVertex2f(x,     y + h);
        glEnd();
    }

    // ── NineSliceImage ───────────────────────────────────────
    // Draws an image with 9-slice scaling
    inline void NineSliceImage(::Image* img,
                               float left, float right, float top, float bottom,
                               float dstX, float dstY, float dstW, float dstH,
                               float tintR = 1, float tintG = 1,
                               float tintB = 1, float tintA = 1)
    {
        NineSlice ns(left, right, top, bottom);
        ns.draw(img, dstX, dstY, dstW, dstH, tintR, tintG, tintB, tintA);
    }

    // ── ImageButton ──────────────────────────────────────────
    // A clickable image. Returns true when clicked.
    inline bool ImageButton(const std::string& id_label,
                            ::Image* img,
                            float x, float y, float w, float h,
                            const Style& st)
    {
        _detail::State& S  = _detail::get();
        std::size_t id     = _detail::hashStr("IB_" + id_label);

        float mx = (float)S.s_mouseX, my = (float)S.s_mouseY;
        bool inside = (mx>=x && mx<=x+w && my>=y && my<=y+h);

        if (inside) S.s_hotId = id;
        if (S.s_hotId == id && S.s_lmbDown && !S.s_lmbDownPrev)
            S.s_activeId = id;

        bool clicked = (S.s_activeId == id && inside &&
                        !S.s_lmbDown && S.s_lmbDownPrev);
        if (clicked) S.s_activeId = 0;

        float tint = (S.s_activeId == id && inside) ? 0.6f :
                     (S.s_hotId    == id)            ? 0.85f : 1.0f;
        Image(img, x, y, w, h, tint, tint, tint, 1.0f);

        // Draw border on hover
        if (S.s_hotId == id)
        {
            glDisable(GL_TEXTURE_2D);
            _drawBorderRect(x, y, w, h,
                            st.borR, st.borG, st.borB, 1.0f,
                            st.borderWidth);
            glEnable(GL_TEXTURE_2D);
        }
        return clicked;
    }

    // Convenience overload — uses the global style
    inline bool ImageButton(const std::string& id_label,
                            ::Image* img,
                            float x, float y, float w, float h)
    { return ImageButton(id_label, img, x, y, w, h, getStyle()); }

    // ── NineSliceImageButton ─────────────────────────────────
    // A clickable 9-slice image button
    inline bool NineSliceImageButton(const std::string& id_label,
                                     ::Image* img,
                                     float left, float right, float top, float bottom,
                                     float x, float y, float w, float h,
                                     const Style& st)
    {
        _detail::State& S  = _detail::get();
        std::size_t id     = _detail::hashStr("NSIB_" + id_label);

        float mx = (float)S.s_mouseX, my = (float)S.s_mouseY;
        bool inside = (mx>=x && mx<=x+w && my>=y && my<=y+h);

        if (inside) S.s_hotId = id;
        if (S.s_hotId == id && S.s_lmbDown && !S.s_lmbDownPrev)
            S.s_activeId = id;

        bool clicked = (S.s_activeId == id && inside &&
                        !S.s_lmbDown && S.s_lmbDownPrev);
        if (clicked) S.s_activeId = 0;

        float tint = (S.s_activeId == id && inside) ? 0.6f :
                     (S.s_hotId    == id)            ? 0.85f : 1.0f;

        NineSlice ns(left, right, top, bottom);
        ns.draw(img, x, y, w, h, tint, tint, tint, 1.0f);

        // Draw border on hover
        if (S.s_hotId == id)
        {
            glDisable(GL_TEXTURE_2D);
            _drawBorderRect(x, y, w, h,
                            st.borR, st.borG, st.borB, 1.0f,
                            st.borderWidth);
            glEnable(GL_TEXTURE_2D);
        }
        return clicked;
    }

    // Convenience overload — uses the global style
    inline bool NineSliceImageButton(const std::string& id_label,
                                     ::Image* img,
                                     float left, float right, float top, float bottom,
                                     float x, float y, float w, float h)
    { return NineSliceImageButton(id_label, img, left, right, top, bottom, x, y, w, h, getStyle()); }

    // ── ProgressBar ──────────────────────────────────────────
    inline void ProgressBar(float progress,   // 0..1
                            float x, float y, float w, float h,
                            float fgR=0.2f, float fgG=0.6f, float fgB=1.0f,
                            float bgR=0.15f,float bgG=0.15f,float bgB=0.2f)
    {
        glDisable(GL_TEXTURE_2D);
        _drawFilledRect(x, y, w, h, bgR, bgG, bgB, 1.0f);
        if (progress > 0.0f)
        {
            float p = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
            _drawFilledRect(x, y, w * p, h, fgR, fgG, fgB, 1.0f);
        }
        _drawBorderRect(x, y, w, h, 0.4f, 0.4f, 0.5f, 1.0f, 1.0f);
        glEnable(GL_TEXTURE_2D);
    }

    // ── Tooltip ──────────────────────────────────────────────
    // Call after the widget it describes. Draws near the mouse.
    inline void Tooltip(const std::string& text, float scale = 1.5f)
    {
        float mx = (float)_detail::get().s_mouseX + 14.0f;
        float my = (float)_detail::get().s_mouseY - 4.0f;
        float tw = _font::textWidth(text, scale) + 10.0f;
        float th = _font::textHeight(scale) + 6.0f;
        glDisable(GL_TEXTURE_2D);
        _drawFilledRect(mx, my, tw, th, 0.08f, 0.08f, 0.12f, 0.95f);
        _drawBorderRect(mx, my, tw, th, 0.5f, 0.5f, 0.6f, 1.0f, 1.0f);
        _font::drawText(text, mx + 5.0f, my + 3.0f, scale,
                        0.95f, 0.95f, 0.95f, 1.0f);
        glEnable(GL_TEXTURE_2D);
    }

    // ── Separator ────────────────────────────────────────────
    inline void Separator(float x, float y, float w,
                          float r=0.4f,float g=0.4f,float b=0.5f,float a=0.8f)
    {
        glDisable(GL_TEXTURE_2D);
        glColor4f(r, g, b, a);
        glBegin(GL_LINES);
            glVertex2f(x,     y);
            glVertex2f(x + w, y);
        glEnd();
        glEnable(GL_TEXTURE_2D);
    }

} // namespace UI

// =============================================================
// Convenience: emit a named UI event from inside a Button call
//
//   if (UIButton("Start", 100,200,160,48))
//       UIEvent("ui_start");
//
//   is exactly:
//   if (UI::Button(...)) EventBus::Emit("ui_start", nullptr, nullptr);
// =============================================================
#define UIButton(label, x, y, w, h, ...)  UI::Button(label,x,y,w,h,##__VA_ARGS__)
#define UILabel(text, x, y, ...)          UI::Label(text,x,y,##__VA_ARGS__)
#define UIEvent(name, ...)                EventBus::Emit(name, ##__VA_ARGS__)
