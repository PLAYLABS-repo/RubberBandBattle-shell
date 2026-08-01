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
struct AnimElement
{
    std::string SpriteName;   // ATLAS_SPRITE_instance or bitmap name
    std::string SymbolName;   // SYMBOL_Instance name

    Vec2  Position  = {0, 0};
    Vec2  BitmapOff = {0, 0}; // bitmap's own local offset inside symbol
    Vec2  Scale     = {1, 1};
    float Rotation  = 0.0f;   // radians (from DecomposedMatrix.Rotation.z)
    Vec2  Pivot     = {0, 0}; // transformationPoint (raw pixels)


    // Graphic symbol playback
    bool  IsGraphic  = false;
    int   FirstFrame = 0;
    bool  Looping    = true;
};

struct AnimFrame
{
    int Index    = 0;
    int Duration = 1;
    std::vector<AnimElement> Elements;
};

struct AnimLayer
{
    std::vector<AnimFrame> Frames;
};

struct AnimTimeline
{
    std::vector<AnimLayer> Layers;
    int TotalFrames = 0;
};

// =========================
// PARENT TRANSFORM
// Describes a world-space anchor that this animator is parented to.
// Set Enabled = true and fill Position/Rotation/Scale to attach.
// =========================
struct AnimParentTransform
{
    bool  Enabled  = false;
    Vec2  Position = {0, 0};
    float Rotation = 0.0f;   // radians
    Vec2  Scale    = {1, 1};
};

// =========================
// MAIN CLASS
// =========================
class Animator
{
public:
    // -------------------------------------------------------
    // Parent transform — set before Draw() to attach to a world anchor.
    //   animator.Parent.Enabled  = true;
    //   animator.Parent.Position = {playerX, playerY};
    //   animator.Parent.Rotation = playerRotRad;
    //   animator.Parent.Scale    = {1, 1};
    // -------------------------------------------------------
    AnimParentTransform Parent;

    bool Load(const char* path);

    // Play a named symbol e.g. Play("PLAYER", "RUN") -> plays PLAYER_ANIM_RUN
    void Play(const std::string& entity, const std::string& animType);

    void Update(float dt);
    void Draw(Image* img, Atlas* atlas, Camera& cam);

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
    // Both functions mutate the stored AnimTimeline in `Symbols`, so the swap
    // persists until you call ChangePart(s) again or reload the file.
    // To revert, call the same function with the arguments swapped.
    // -------------------------------------------------------
    void ChangePart (const std::string& oldSprite,
                     const std::string& newSprite,
                     const std::string& animKey);

    void ChangeParts(const std::string& oldSprite,
                     const std::string& newSprite,
                     const std::string& animKey);

    int  CurrentFrame = 0;
    int  TotalFrames  = 0;

private:
    std::map<std::string, AnimTimeline> Symbols;
    AnimTimeline* ActiveAnim = nullptr;

    float FrameTimer = 0.0f;
    float Fps        = 30.0f;

    void DrawAnim(
        AnimTimeline& timeline,
        Image*        img,
        Atlas*        atlas,
        Vec2          parentPos,
        float         parentRot,   // radians
        Vec2          parentScale,
        int           frame
    );

    void DrawSprite(
        const std::string& name,
        Image* img, Atlas* atlas,
        Vec2 pos, float rotRad, Vec2 scale, Vec2 pivot,
        Vec2 bitmapOff
    );

    // Internal helpers for ChangePart / ChangeParts
    void SwapSpriteInAnim(AnimTimeline&       timeline,
                           const std::string& oldSprite,
                           const std::string& newSprite,
                           bool               recursive);

    void SwapSpriteInElement(AnimElement&        el,
                              const std::string& oldSprite,
                              const std::string& newSprite,
                              bool               recursive);
};
