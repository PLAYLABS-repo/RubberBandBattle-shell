#pragma once

// ================================================================
//  ai.h  –  Simple steering AI for playFramework
//  Works with Sprite* and Shape* from the framework.
//
//  KEY DESIGN NOTES (read before touching this file):
//  ─────────────────────────────────────────────────
//  • Sprite already has its own lerp system:
//      update(dt) slides  position → targetPosition
//    So we NEVER touch sprite->position directly.
//    We only set  sprite->targetPosition  (and optionally moveSpeed).
//
//  • Shape has no built-in lerp.  We move shape->x / shape->y
//    directly every frame (same as the game does in main.cpp).
//
//  • Call ai.update(dt) BEFORE sprite->update(dt) each frame so
//    the target is fresh when the sprite lerps toward it.
// ================================================================

#include "Vec2.h"

class Sprite;
class Shape;

// ----------------------------------------------------------------
enum class AIState
{
    IDLE,     // do nothing
    SEEK,     // chase target at full speed
    FLEE,     // run away from target
    ARRIVE,   // chase but slow down near the target
    PATROL,   // loop through waypoints
};

// ----------------------------------------------------------------
class AI
{
public:
    // ── tuneable ────────────────────────────────────────────────
    float speed        = 150.0f;  // pixels/sec  (used for Shape)
    float arriveRadius = 80.0f;   // begin slowing inside this distance
    float stopRadius   = 6.0f;    // "close enough" snap threshold

    // ── attachment ──────────────────────────────────────────────
    // Call ONE of these before using the AI.
    void attachSprite(Sprite* s);
    void attachShape (Shape*  s);

    // ── commands ────────────────────────────────────────────────

    // Fixed world-position targets
    void seekPosition  (float x, float y);
    void fleePosition  (float x, float y);
    void arrivePosition(float x, float y);

    // Live-target: AI reads the target's position every frame
    void seekSprite (Sprite* target);
    void seekShape  (Shape*  target);
    void fleeSprite (Sprite* target);
    void fleeShape  (Shape*  target);

    // Patrol – add waypoints then call startPatrol()
    void addWaypoint  (float x, float y);
    void clearWaypoints();
    void startPatrol  ();

    // Idle / state
    void    idle();
    void    setState(AIState s);
    AIState getState() const { return state_; }

    // ── per-frame ───────────────────────────────────────────────
    // Call this once per game loop tick.
    void update(float dt);

    // ── queries ─────────────────────────────────────────────────
    bool hasArrived() const { return arrived_; }
    Vec2 position()   const;   // returns current world position of attached entity

private:
    Sprite* sprite_ = nullptr;
    Shape*  shape_  = nullptr;

    AIState state_     = AIState::IDLE;
    Vec2    targetPos_ = {0, 0};
    bool    arrived_   = false;

    // live-follow targets (optional – null means fixed targetPos_)
    Sprite* followSprite_ = nullptr;
    Shape*  followShape_  = nullptr;

    // patrol
    static const int MAX_WP = 32;
    float wpX_[MAX_WP] = {};
    float wpY_[MAX_WP] = {};
    int   wpCount_     = 0;
    int   wpIndex_     = 0;

    // ── private helpers ─────────────────────────────────────────
    Vec2 entityPos()                         const;
    void applyTarget(float tx, float ty);    // sets sprite target or shape pos
    void syncLiveTarget();                   // refreshes targetPos_ from follow-pointer
    void doSeek  (float tx, float ty, float dt, bool arrive);
    void doFlee  (float tx, float ty, float dt);
};
