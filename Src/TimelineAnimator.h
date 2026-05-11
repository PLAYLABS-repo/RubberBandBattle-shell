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

    int  currentFrame = 0;
    int  totalFrames  = 0;

private:
    std::map<std::string, TA_Timeline> symbols;
    TA_Timeline* activeTimeline = nullptr;

    float frameTimer = 0.0f;
    float fps        = 24.0f;

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
};
