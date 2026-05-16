#include "Player.h"

void Player::HandleInput()
{
    vx = 0;

    if (KeyDown(VK_LEFT) || KeyDown('A'))
    {
        vx = -speed;
        facingX = -1;
    }

    if (KeyDown(VK_RIGHT) || KeyDown('D'))
    {
        vx = speed;
        facingX = 1;
    }

    if (KeyPressed(VK_SPACE) && onGround)
    {
        vy = jumpForce;
        onGround = false;
        state = State::JUMP;
    }
}

void Player::Update(float dt)
{
    x += vx * dt;

    if (!onGround)
    {
        vy += gravity * dt;
        y  += vy * dt;

        if (y >= 436.0f)
        {
            y = 436.0f;
            vy = 0;
            onGround = true;
        }
    }

    sprite->position = {x, y};
    sprite->scale = {facingX, 1.0f};
}
