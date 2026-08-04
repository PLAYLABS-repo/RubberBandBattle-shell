#pragma once

#include "DrawHelpers.h"

class Sprite;
class TimelineAnimator;
class Window;

// ============================================================
// Player
// Pure movement: walk/run/jump, stamina, gravity, wall collision,
// and the animation state that follows from them. No combat,
// no health, no death/respawn.
// ============================================================
struct Player
{
    enum class State { IDLE, WALK, SWIM, JUMP, DIE, RUN,HURT,SHOOT,SHOOT_RUN, SLIDE,PUNCH};
float  playerHealth = 100;
bool canMove = true;
    float x = 0.0f, y = 0.0f;
    float w = 100.0f, h = 200.0f;
    float minY = 0.0f, maxY = 700.0f;
    float minX = 0.0f, maxX = 2500.0f;
    float baseY = 0.0f;
    std::string username;

    float velocityY = 0.0f;
    float gravity   = 2000.0f;
    float jumpForce =  -800.0f;
    float speed     =  300.0f;
    bool  jumping   = false;
   Image* playersheet  ;
    Atlas* playeratlas  ;
    Animator* anim  ;


    Vec2 position;

    float facingX = 1.0f;

    // ---- Stamina (gates running) ----
    static constexpr float MAX_STAMINA       = 100.0f;
    static constexpr float STAMINA_RUN_DRAIN =  20.0f; // per second while running
    static constexpr float STAMINA_JMP_DRAIN =  15.0f; // flat cost per jump
    static constexpr float STAMINA_RECOVER   =  12.0f; // per second when not running
    float stamina          = MAX_STAMINA;
    bool  staminaExhausted = false;

    // ---- Animation / visuals ----
    State     lastAnimState = State::IDLE;

    Sprite*   sprite = nullptr;

    // ---- Per-frame input snapshot; main() fills this in from KeyDown/KeyPressed ----
    struct Input
    {
        bool moveLeft = false, moveRight = false;
        bool moveUp   = false, moveDown  = false;
        bool shiftHeld   = false;
        bool jumpPressed = false; // KeyPressed(VK_SPACE) this frame
    };

    // Things that happened this frame that main() may want to react to (sfx, Event() calls)
    struct FrameEvents
    {
        bool jumped = false;
        bool landed = false;
    };

    // Polls the window and reads the movement/run/jump keys into an Input
    // snapshot for this frame. Call once per frame before update().
    static Input gatherInput(Window& window);

    // Runs movement input, stamina, jump trigger, gravity/physics,
    // wall collision, and animation-state selection for one frame.
    // wallBox is the obstacle the player can land on / bump into.
    FrameEvents update(float dt, const Input& input, const AABB& wallBox);

    // Places the player at a given position with a clean movement state.
    void resetToSpawn(float spawnX, float spawnY);

private:
    void applyAnimationState(bool moving, bool canRun);
    void applyPhysics(float dt, float moveX, float moveY, bool moving, bool canRun, bool& outLanded);
    void resolveWallCollision(const AABB& wallBox, float moveX, float currentSpeed, float dt,
                              bool moving, bool canRun, bool& outLanded);
};
