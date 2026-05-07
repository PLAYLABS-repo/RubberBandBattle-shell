#pragma once
// =============================================================
// Camera3D.h — 3D perspective camera
// Stores view + projection matrices in column-major float[16]
// so they can be passed directly to glLoadMatrixf / shaders.
// =============================================================

#include "Mat4.h"
#include "Vec3.h"

struct Camera3D
{
    // ── Matrices (column-major, OpenGL convention) ────────────
    float view[16];        // updated by LookAt
    float projection[16];  // updated by Perspective / Ortho

    // ── Stored params (for recalculation on resize etc.) ─────
    float eyeX  = 0, eyeY = 0, eyeZ = 5;
    float fovY  = 60.0f;   // degrees
    float nearZ = 0.1f;
    float farZ  = 1000.0f;

    Camera3D()
    {
        Mat4 id = Mat4::Identity();
        for (int i=0;i<16;++i) view[i]       = id.m[i];
        for (int i=0;i<16;++i) projection[i] = id.m[i];
    }
};
