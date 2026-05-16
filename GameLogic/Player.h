#pragma once

#include "PlaylabsGL.h"
#include "Src/Shape.h"

struct Player
{
    enum class State
    {
        IDLE,
        RUN,
        JUMP,
        PUNCH,
        DEAD
    };

    float x = 200.0f;
    float y = 436.0f;
    float w = 64.0f;
    float h = 64.0f;
    float speed = 250.0f;

    float minY = 100.0f;
    float maxY = 500.0f;

    float baseY = 436.0f;
    float velocityY = 0.0f;
    float gravity = 900.0f;
    float jumpForce = -520.0f;
    bool jumping = false;

    float chargeTimer = 0.0f;
    float shootCooldown = 0.0f;
    int ammo = 25;

    int hp = 100;
    int maxHp = 100;

    float aimDirX = 1.0f;
    float aimDirY = 0.0f;

    float facingX = 1.0f;

    bool punching = false;
    float punchTimer = 0.0f;
    float punchCooldown = 0.0f;
    bool punchHit = false;

    Sprite* sprite = nullptr;
    TimelineAnimator* anim = nullptr;

    State state = State::IDLE;

    float muzzleX() const { return x + w * 0.5f; }
    float muzzleY() const { return y + h * 0.5f; }

    void getPunchHitbox(float& hx, float& hy, float& hw, float& hh) const
    {
        const float PUNCH_RANGE = 80.0f;

        hw = PUNCH_RANGE;
        hh = 200.0f;

        hx = (facingX < 0.0f) ? x + w : x - hw;
        hy = y + h;
    }

    // =========================================================
    // UPDATE (PlaylabsGL only)
    // =========================================================
    void Update(float dt)
    {
        float move = speed * dt;

        // movement
        if (KeyDown('A'))
        {
            x -= move;
            facingX = -1.0f;
            state = State::RUN;
        }
        if (KeyDown('D'))
        {
            x += move;
            facingX = 1.0f;
            state = State::RUN;
        }

        if (!KeyDown('A') && !KeyDown('D'))
            state = State::IDLE;

        // jump
        if (KeyDown('W') && !jumping)
        {
            velocityY = jumpForce;
            jumping = true;
            state = State::JUMP;
        }

        // gravity
        velocityY += gravity * dt;
        y += velocityY * dt;

        // ground collision
        if (y >= baseY)
        {
            y = baseY;
            velocityY = 0.0f;
            jumping = false;
        }

        // punch cooldown
        if (punchCooldown > 0.0f)
            punchCooldown -= dt;

        if (KeyDown(' ') && punchCooldown <= 0.0f)
        {
            punching = true;
            punchTimer = 0.15f;
            punchCooldown = 0.4f;
            state = State::PUNCH;
        }

        if (punching)
        {
            punchTimer -= dt;
            if (punchTimer <= 0.0f)
            {
                punching = false;
            }
        }

        // shooting cooldown
        if (shootCooldown > 0.0f)
            shootCooldown -= dt;

        if (KeyDown('F') && shootCooldown <= 0.0f && ammo > 0)
        {
            ammo--;
            shootCooldown = 0.25f;
        }
    }

    // =========================================================
    // DRAW (PlaylabsGL only)
    // =========================================================
    void Draw()
    {
        // fallback visual if no sprite/anim
        if (!sprite)
        {
            //drawRect(x, y, w, h);
        }
        else
        {
            // if you later hook animator:
            // TickAnimator(anim, dt, image, atlas, camera);
        }
    }
};
