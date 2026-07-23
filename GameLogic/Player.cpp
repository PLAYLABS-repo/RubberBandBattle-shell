#include "Player.h"
#include "PlaylabsGL.h"
#include <windows.h>
#include <algorithm>
constexpr float Player::MAX_STAMINA;
constexpr float Player::STAMINA_RUN_DRAIN;
constexpr float Player::STAMINA_JMP_DRAIN;
constexpr float Player::STAMINA_RECOVER;
Player::Input Player::gatherInput(Window& window)
{
    PollInput(&window);

    Input in;
    in.moveRight   = KeyDown(VK_RIGHT) || KeyDown('D');
    in.moveLeft    = KeyDown(VK_LEFT)  || KeyDown('A');
    in.moveUp      = KeyDown(VK_UP)    || KeyDown('W');
    in.moveDown    = KeyDown(VK_DOWN)  || KeyDown('S');
    in.shiftHeld   = KeyDown(VK_SHIFT) != 0;
    in.jumpPressed = KeyPressed(VK_SPACE) != 0;
    return in;
}


void Player::applyAnimationState(bool moving, bool canRun)
{
    if (jumping) return;

    if (moving && canRun && lastAnimState != State::RUN)
    {
        Anim(anim, PLAYER, RUN);
        lastAnimState = State::RUN;
    }
    else if (moving && !canRun && lastAnimState != State::WALK)
    {
        Anim(anim, PLAYER, WALK);
        lastAnimState = State::WALK;
    }
    else if (!moving && lastAnimState != State::IDLE)
    {
        Anim(anim, PLAYER, IDLE);
        lastAnimState = State::IDLE;
    }
}

void Player::applyPhysics(float dt, float moveX, float moveY, bool moving, bool canRun, bool& outLanded)
{
    x += moveX * (canRun ? speed * 1.7f : speed) * dt;

    if (!jumping)
    {
        y += moveY * (canRun ? speed * 1.7f : speed) * dt;
        y  = std::max(minY, std::min(y, maxY - h));
        baseY = y;
        return;
    }

    velocityY += gravity * dt;
    y         += velocityY * dt;

    if (y >= baseY)
    {
        y         = baseY;
        velocityY = 0.0f;
        jumping   = false;
        outLanded = true;
        applyAnimationState(moving, canRun);
    }
}

void Player::resolveWallCollision(const AABB& wallBox, float moveX, float currentSpeed, float dt,
                                   bool moving, bool canRun, bool& outLanded)
{
    float phbX = x + (w - 100.0f) * 0.5f, phbY = y + h;
    float phbW = jumping ? 60.0f : 100.0f, phbH = jumping ? 100.0f : 200.0f;

    AABB playerBox(phbX, phbY, phbW, phbH);
    CollisionSide side = playerBox.getCollisionSide(wallBox);

    switch (side)
    {
        case CollisionSide::Top:
            y         = wallBox.y - phbH - h;
            baseY     = y;
            velocityY = 0.0f;
            jumping   = false;
            applyAnimationState(moving, canRun);
            outLanded = true;
            break;

        case CollisionSide::Bottom:
            velocityY = 0.0f;
            y         = wallBox.y + wallBox.h - h;
            break;

        case CollisionSide::Left:
        case CollisionSide::Right:
            x -= moveX * currentSpeed * dt;
            break;

        case CollisionSide::None:
            break;
    }
}

Player::FrameEvents Player::update(float dt, const Input& input, const AABB& wallBox)
{
    FrameEvents events;

    // ---- Movement input ----
    bool  moving = false;
    float moveX = 0.0f, moveY = 0.0f;

    if (input.moveRight) { moveX =  1.0f; moving = true; facingX = -1.0f; }
    if (input.moveLeft)  { moveX = -1.0f; moving = true; facingX = 1.0f; }

    if (!jumping)
    {
        if (input.moveUp)   { moveY = -1.0f; moving = true; }
        if (input.moveDown) { moveY =  1.0f; moving = true; }
    }

    // ---- Stamina / run gating ----
    if (staminaExhausted && stamina >= MAX_STAMINA)
        staminaExhausted = false;

    bool canRun = input.shiftHeld && !staminaExhausted;

   if (canRun && moving)
{
    stamina -= STAMINA_RUN_DRAIN * dt;
    if (stamina <= 0.0f)
    {
        stamina          = 0.0f;
        staminaExhausted = true;
        canRun           = false;
    }
}
else
{
    stamina = std::min(stamina + Player::STAMINA_RECOVER * dt, MAX_STAMINA);
}

    float currentSpeed = canRun ? speed * 1.7f : speed;

    // ---- Jump ----
    if (input.jumpPressed && !jumping)
    {
        baseY     = y;
        velocityY = jumpForce;
        jumping   = true;

        stamina -= STAMINA_JMP_DRAIN;
        if (stamina <= 0.0f)
        {
            stamina          = 0.0f;
            staminaExhausted = true;
        }

        Anim(anim, PLAYER, JUMP);
        lastAnimState = State::JUMP;
        events.jumped = true;
    }

    // ---- Physics ----
    bool landed = false;
    applyPhysics(dt, moveX, moveY, moving, canRun, landed);

    // ---- Sprite transform sync (caller ticks the sprite/animator itself) ----
    if (sprite)
    {
        sprite->position       = {x, y};
        sprite->targetPosition = {x, y};
        sprite->scale          = {facingX, 1.0f};
        sprite->targetScale    = {facingX, 1.0f};
    }

    SetAnimatorParent(anim, x, y, 0.0f, facingX, 1.0f);
    applyAnimationState(moving, canRun);

    // ---- Wall collision ----
    resolveWallCollision(wallBox, moveX, currentSpeed, dt, moving, canRun, landed);

    if (sprite)
    {
        sprite->position       = {x, y};
        sprite->targetPosition = {x, y};
    }

    events.landed = events.landed || landed;
    return events;
}

void Player::resetToSpawn(float spawnX, float spawnY)
{
    x     = spawnX;
    y     = spawnY;
    baseY = spawnY;

    velocityY = 0.0f;
    jumping   = false;

    stamina          = MAX_STAMINA;
    staminaExhausted = false;

    if (sprite)
    {
        sprite->position       = {x, y};
        sprite->targetPosition = {x, y};
    }

    Anim(anim, "PLAYER", "IDLE");
    lastAnimState = State::IDLE;
}
