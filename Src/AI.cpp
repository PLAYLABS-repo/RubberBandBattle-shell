#include "ai.h"
#include "Sprite.h"
#include "Shape.h"
#include <cmath>

// ================================================================
//  ai.cpp  –  implementation
// ================================================================

// ── small math helpers ──────────────────────────────────────────

static inline float length2(float dx, float dy)
{
    return sqrtf(dx * dx + dy * dy);
}

// ── attachment ──────────────────────────────────────────────────

void AI::attachSprite(Sprite* s)
{
    sprite_ = s;
    shape_  = nullptr;
}

void AI::attachShape(Shape* s)
{
    shape_  = s;
    sprite_ = nullptr;
}

// ── commands: fixed position ────────────────────────────────────

void AI::seekPosition(float x, float y)
{
    targetPos_    = {x, y};
    followSprite_ = nullptr;
    followShape_  = nullptr;
    state_        = AIState::SEEK;
    arrived_      = false;
}

void AI::fleePosition(float x, float y)
{
    targetPos_    = {x, y};
    followSprite_ = nullptr;
    followShape_  = nullptr;
    state_        = AIState::FLEE;
    arrived_      = false;
}

void AI::arrivePosition(float x, float y)
{
    targetPos_    = {x, y};
    followSprite_ = nullptr;
    followShape_  = nullptr;
    state_        = AIState::ARRIVE;
    arrived_      = false;
}

// ── commands: live targets ───────────────────────────────────────

void AI::seekSprite(Sprite* target)
{
    followSprite_ = target;
    followShape_  = nullptr;
    state_        = AIState::SEEK;
    arrived_      = false;
}

void AI::seekShape(Shape* target)
{
    followShape_  = target;
    followSprite_ = nullptr;
    state_        = AIState::SEEK;
    arrived_      = false;
}

void AI::fleeSprite(Sprite* target)
{
    followSprite_ = target;
    followShape_  = nullptr;
    state_        = AIState::FLEE;
    arrived_      = false;
}

void AI::fleeShape(Shape* target)
{
    followShape_  = target;
    followSprite_ = nullptr;
    state_        = AIState::FLEE;
    arrived_      = false;
}

// ── patrol ───────────────────────────────────────────────────────

void AI::addWaypoint(float x, float y)
{
    if (wpCount_ < MAX_WP)
    {
        wpX_[wpCount_] = x;
        wpY_[wpCount_] = y;
        ++wpCount_;
    }
}

void AI::clearWaypoints()
{
    wpCount_ = 0;
    wpIndex_ = 0;
}

void AI::startPatrol()
{
    if (wpCount_ == 0) return;
    wpIndex_      = 0;
    targetPos_    = {wpX_[0], wpY_[0]};
    followSprite_ = nullptr;
    followShape_  = nullptr;
    state_        = AIState::PATROL;
    arrived_      = false;
}

// ── state ────────────────────────────────────────────────────────

void AI::idle()
{
    state_ = AIState::IDLE;
}

void AI::setState(AIState s)
{
    state_   = s;
    arrived_ = false;
}

// ── per-frame update ─────────────────────────────────────────────

void AI::update(float dt)
{
    if (state_ == AIState::IDLE) return;
    if (!sprite_ && !shape_)    return;

    // pull live-target position into targetPos_ if set
    syncLiveTarget();

    switch (state_)
    {
    case AIState::SEEK:
        doSeek(targetPos_.x, targetPos_.y, dt, false);
        break;

    case AIState::ARRIVE:
        doSeek(targetPos_.x, targetPos_.y, dt, true);
        break;

    case AIState::FLEE:
        doFlee(targetPos_.x, targetPos_.y, dt);
        break;

    case AIState::PATROL:
    {
        if (wpCount_ == 0) break;

        doSeek(targetPos_.x, targetPos_.y, dt, false);

        // reached current waypoint → advance to next
        Vec2  cur = entityPos();
        float dx  = targetPos_.x - cur.x;
        float dy  = targetPos_.y - cur.y;
        if (length2(dx, dy) <= stopRadius)
        {
            wpIndex_   = (wpIndex_ + 1) % wpCount_;
            targetPos_ = {wpX_[wpIndex_], wpY_[wpIndex_]};
        }
        break;
    }

    default: break;
    }
}

// ── queries ──────────────────────────────────────────────────────

Vec2 AI::position() const
{
    return entityPos();
}

// ── private helpers ──────────────────────────────────────────────

// Returns the current world position of the attached entity.
Vec2 AI::entityPos() const
{
    if (sprite_) return sprite_->position;   // use rendered position, not target
    if (shape_)  return {shape_->x, shape_->y};
    return {0, 0};
}

// Pushes a destination to the attached entity.
//
// SPRITE: sets targetPosition (+ adjusts moveSpeed to honour AI::speed)
//         then Sprite::update(dt) does the smooth lerp automatically.
//
// SHAPE:  no lerp system, so we move x/y directly here (called from
//         doSeek/doFlee which already compute a per-frame delta).
//
// NOTE:   For Sprite we only call this once to set the target;
//         the delta-movement is handled inside doSeek / doFlee by
//         calling applyTarget with the DESTINATION, not a delta.
void AI::applyTarget(float tx, float ty)
{
    if (sprite_)
    {
        // Sprite's built-in lerp uses moveSpeed (pixels/sec).
        // We override it here so the AI speed setting is respected.
        sprite_->moveSpeed      = speed;
        sprite_->targetPosition = {tx, ty};
    }
    else if (shape_)
    {
        shape_->x = tx;
        shape_->y = ty;
    }
}

// Sync targetPos_ from a live follow-pointer each frame.
void AI::syncLiveTarget()
{
    if (followSprite_)
    {
        targetPos_.x = followSprite_->position.x;
        targetPos_.y = followSprite_->position.y;
    }
    else if (followShape_)
    {
        targetPos_.x = followShape_->x;
        targetPos_.y = followShape_->y;
    }
}

// Move (or point) toward (tx, ty).
// arrive=true → scale speed down inside arriveRadius.
void AI::doSeek(float tx, float ty, float dt, bool arrive)
{
    Vec2  cur  = entityPos();
    float dx   = tx - cur.x;
    float dy   = ty - cur.y;
    float dist = length2(dx, dy);

    if (dist <= stopRadius)
    {
        arrived_ = true;
        applyTarget(tx, ty);   // snap
        return;
    }

    arrived_ = false;

    float s = speed;
    if (arrive && dist < arriveRadius)
        s = speed * (dist / arriveRadius);

    // For SHAPE we compute the new position directly (no built-in lerp).
    // For SPRITE we set targetPosition to a point clamped one frame ahead
    // so the sprite's own lerp drives the motion at the right speed.
    float nx = dx / dist;
    float ny = dy / dist;

    if (shape_)
    {
        float step = s * dt;
        if (step > dist) step = dist;   // don't overshoot
        applyTarget(cur.x + nx * step, cur.y + ny * step);
    }
    else if (sprite_)
    {
        // Point the Sprite's lerp engine straight at the destination.
        // Sprite::update(dt) will chase it at moveSpeed (= AI::speed).
        applyTarget(tx, ty);
    }
}

// Move away from (tx, ty).
void AI::doFlee(float tx, float ty, float dt)
{
    Vec2  cur  = entityPos();
    float dx   = cur.x - tx;   // reversed: away from target
    float dy   = cur.y - ty;
    float dist = length2(dx, dy);

    if (dist < 0.001f) { dx = 1.0f; dy = 0.0f; dist = 1.0f; } // safety

    float nx = dx / dist;
    float ny = dy / dist;

    if (shape_)
    {
        applyTarget(cur.x + nx * speed * dt,
                    cur.y + ny * speed * dt);
    }
    else if (sprite_)
    {
        // Project a flee destination far away in the away-direction
        // so the Sprite always has a valid target to lerp toward.
        const float FLEE_LOOKAHEAD = 400.0f;
        applyTarget(cur.x + nx * FLEE_LOOKAHEAD,
                    cur.y + ny * FLEE_LOOKAHEAD);
    }
}
