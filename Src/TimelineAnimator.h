#pragma once
#include <string>
#include <vector>
#include <map>
#include "Vec2.h"

class Image;
class Atlas;
class Camera;

// =========================
// STRUCTS
// =========================
struct TA_Element
{
    std::string spriteName;   // ATLAS_SPRITE_instance or bitmap name
    std::string symbolName;   // SYMBOL_Instance name

    Vec2  position  = {0, 0};
    Vec2  bitmapOff = {0, 0}; // bitmap's own local offset inside symbol
    Vec2  scale     = {1, 1};
    float rotation  = 0.0f;   // radians (from DecomposedMatrix.Rotation.z)
    Vec2  pivot     = {0, 0}; // transformationPoint (raw pixels)

    // Graphic symbol playback
    bool  isGraphic  = false;
    int   firstFrame = 0;
    bool  looping    = true;
};

struct TA_Frame
{
    int index    = 0;
    int duration = 1;
    std::vector<TA_Element> elements;
};

struct TA_Layer
{
    std::vector<TA_Frame> frames;
};

struct TA_Timeline
{
    std::vector<TA_Layer> layers;
    int totalFrames = 0;
};

// =========================
// PARENT TRANSFORM
// Describes a world-space anchor that this animator is parented to.
// Set parentEnabled = true and fill position/rotation/scale to attach.
// =========================
struct TA_ParentTransform
{
    bool  enabled  = false;
    Vec2  position = {0, 0};
    float rotation = 0.0f;   // radians
    Vec2  scale    = {1, 1};
};

// =========================
// MAIN CLASS
// =========================
class TimelineAnimator
{
public:
    // -------------------------------------------------------
    // Parent transform — set before draw() to attach to a world anchor.
    //   animator.parent.enabled  = true;
    //   animator.parent.position = {playerX, playerY};
    //   animator.parent.rotation = playerRotRad;
    //   animator.parent.scale    = {1, 1};
    // -------------------------------------------------------
    TA_ParentTransform parent;

    bool load(const char* path);

    // Play a named symbol e.g. play("PLAYER", "RUN") -> plays PLAYER_ANIM_RUN
    void play(const std::string& entity, const std::string& animType);

    void update(float dt);
    void draw(Image* img, Atlas* atlas, Camera& cam);

    // -------------------------------------------------------
    // Part swapping — replace a sprite name inside a specific animation.
    //
    // ChangePart  — swaps only inside the TOP-LEVEL timeline of animKey.
    //               Use when the sprite sits directly on the root timeline.
    //               e.g. ChangePart("head_default", "head_helmet", "PLAYER_ANIM_RUN")
    //
    // ChangeParts — swaps inside animKey AND every nested symbol it references,
    //               recursively. Use when the sprite may live inside a
    //               sub-symbol (body part, limb, accessory, etc.).
    //               e.g. ChangeParts("sword_a", "sword_b", "PLAYER_ANIM_RUN")
    //
    // Both functions mutate the stored TA_Timeline in `symbols`, so the swap
    // persists until you call ChangePart(s) again or reload the file.
    // To revert, call the same function with the arguments swapped.
    // -------------------------------------------------------
    void ChangePart (const std::string& oldSprite,
                     const std::string& newSprite,
                     const std::string& animKey);

    void ChangeParts(const std::string& oldSprite,
                     const std::string& newSprite,
                     const std::string& animKey);

    int  currentFrame = 0;
    int  totalFrames  = 0;

private:
    std::map<std::string, TA_Timeline> symbols;
    TA_Timeline* activeTimeline = nullptr;

    float frameTimer = 0.0f;
    float fps        = 30.0f;

    void drawTimeline(
        TA_Timeline& timeline,
        Image*       img,
        Atlas*       atlas,
        Vec2         parentPos,
        float        parentRot,   // radians
        Vec2         parentScale,
        int          frame
    );

    void drawSprite(
        const std::string& name,
        Image* img, Atlas* atlas,
        Vec2 pos, float rotRad, Vec2 scale, Vec2 pivot,
        Vec2 bitmapOff
    );

    // Internal helpers for ChangePart / ChangeParts
    void swapSpriteInTimeline(TA_Timeline&       timeline,
                              const std::string& oldSprite,
                              const std::string& newSprite,
                              bool               recursive);

    void swapSpriteInElement (TA_Element&        el,
                              const std::string& oldSprite,
                              const std::string& newSprite,
                              bool               recursive);
};
