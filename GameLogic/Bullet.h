#pragma once
#include "PlaylabsGL.h"
#include "Constants.h"

// =============================================================
// BULLET
// =============================================================
struct Bullet
{
    float x, y;
    float velX, velY;
    float w    = 48.0f;
    float h    = 48.0f;
    float hitW = 40.0f;
    float hitH = 40.0f;
    float life = BULLET_LIFETIME;
    bool  dead = false;

    Sprite* sprite = nullptr;

    float hitX() const { return x + (w - hitW) * 0.5f; }
    float hitY() const { return y + (h - hitH) * 0.5f; }
};
