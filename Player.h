#pragma once
#include "PlaylabsGL.h"
#include "Constants.h"

struct Player
{
    float x     = 200.0f;
    float y     = 436.0f;
    float w     = 64.0f;
    float h     = 64.0f;
    float speed = 250.0f;

    float minY = 100.0f;
    float maxY = 500.0f;

    float baseY     = 436.0f;
    float velocityY = 0.0f;
    float gravity   = 900.0f;
    float jumpForce = -520.0f;
    bool  jumping   = false;

    bool  charging      = false;
    float chargeTimer   = 0.0f;
    float shootCooldown = 0.0f;
    int   ammo          = MAX_AMMO;

    float aimDirX = 1.0f;
    float aimDirY = 0.0f;

    bool  punching      = false;
    float punchTimer    = 0.0f;
    float punchCooldown = 0.0f;
    bool  punchHit      = false;

    Sprite*           sprite = nullptr;
    TimelineAnimator* anim   = nullptr;

    enum class State { IDLE, RUN, JUMP, PUNCH } state = State::IDLE;
    float facingX = 1.0f;

    float muzzleX() const { return x + w * 0.5f; }
    float muzzleY() const { return y + h * 0.5f; }

    void getPunchHitbox(float& hx, float& hy, float& hw, float& hh) const
    {
        hw = PUNCH_RANGE;
        hh = 50.0f;
        if (facingX < 0.0f)
            hx = x + w;
        else
            hx = x - hw;
        hy = y + h * 0.2f;
    }
};
