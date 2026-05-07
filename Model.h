#pragma once
// =============================================================
// Model.h — 3D model: mesh + texture + per-instance transform
// =============================================================

#include "Mesh.h"
#include "Image.h"

struct Model
{
    // ── Resources (borrowed — Model does NOT own these) ───────
    Mesh*  mesh    = nullptr;
    Image* diffuse = nullptr;   // optional texture

    // ── Camera bind ──────────────────────────────────────────
    // true  → full MVP transform  (world-space object, default)
    // false → strips camera translation (skybox, screen overlay)
    bool cameraBind = true;

    // ── Transform (TRS) ──────────────────────────────────────
    float tx = 0, ty = 0, tz = 0;        // translation
    float rx = 0, ry = 1, rz = 0;        // rotation axis
    float angleDegrees = 0.0f;           // rotation angle
    float sx = 1, sy = 1, sz = 1;        // scale
};
