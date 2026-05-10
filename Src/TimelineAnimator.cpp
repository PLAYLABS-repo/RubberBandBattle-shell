#include "TimelineAnimator.h"
#include "Image.h"
#include "Atlas.h"
#include "Camera.h"

#include <GL/gl.h>
#include <fstream>
#include <cmath>
#include "json.hpp"

using json = nlohmann::json;

// =========================
// HELPERS
// =========================
static float safeFloat(const json& j, const char* key, float fallback = 0.0f)
{
    if (!j.contains(key) || j[key].is_null()) return fallback;
    return j[key].get<float>();
}

static int safeInt(const json& j, const char* key, int fallback = 0)
{
    if (!j.contains(key) || j[key].is_null()) return fallback;
    return j[key].get<int>();
}

static std::string safeString(const json& j, const char* key, const std::string& fallback = "")
{
    if (!j.contains(key) || j[key].is_null()) return fallback;
    return j[key].get<std::string>();
}

// =========================
// PARSE ELEMENT
// Three element types:
//   1. ATLAS_SPRITE_instance  — leaf sprite
//   2. SYMBOL_Instance (with bitmap field) — leaf wrapped in symbol
//   3. SYMBOL_Instance (no bitmap) — nested movieclip/graphic, recurse
// =========================
static TA_Element parseElement(const json& e)
{
    TA_Element el;

    if (e.contains("ATLAS_SPRITE_instance") && !e["ATLAS_SPRITE_instance"].is_null())
    {
        auto& sp = e["ATLAS_SPRITE_instance"];
        el.spriteName = safeString(sp, "name");

        if (sp.contains("Position") && !sp["Position"].is_null())
        {
            el.position.x = safeFloat(sp["Position"], "x");
            el.position.y = safeFloat(sp["Position"], "y");
        }
        if (sp.contains("transformationPoint") && !sp["transformationPoint"].is_null())
        {
            el.pivot.x = safeFloat(sp["transformationPoint"], "x");
            el.pivot.y = safeFloat(sp["transformationPoint"], "y");
        }
    }

    if (e.contains("SYMBOL_Instance") && !e["SYMBOL_Instance"].is_null())
    {
        auto& inst = e["SYMBOL_Instance"];

        if (inst.contains("DecomposedMatrix") && !inst["DecomposedMatrix"].is_null())
        {
            auto& dm = inst["DecomposedMatrix"];
            if (dm.contains("Position") && !dm["Position"].is_null())
            {
                el.position.x = safeFloat(dm["Position"], "x");
                el.position.y = safeFloat(dm["Position"], "y");
            }
            if (dm.contains("Scaling") && !dm["Scaling"].is_null())
            {
                el.scale.x = safeFloat(dm["Scaling"], "x", 1.0f);
                el.scale.y = safeFloat(dm["Scaling"], "y", 1.0f);
            }
            if (dm.contains("Rotation") && !dm["Rotation"].is_null())
            {
                el.rotation = safeFloat(dm["Rotation"], "z");
            }
        }

        if (inst.contains("transformationPoint") && !inst["transformationPoint"].is_null())
        {
            el.pivot.x = safeFloat(inst["transformationPoint"], "x");
            el.pivot.y = safeFloat(inst["transformationPoint"], "y");
        }

        if (inst.contains("bitmap") && !inst["bitmap"].is_null())
        {
            auto& bm = inst["bitmap"];
            el.spriteName  = safeString(bm, "name");
            el.bitmapOff.x = safeFloat(bm.contains("Position") ? bm["Position"] : json{}, "x");
            el.bitmapOff.y = safeFloat(bm.contains("Position") ? bm["Position"] : json{}, "y");
        }
        else
        {
            el.symbolName = safeString(inst, "SYMBOL_name");

            std::string sType = safeString(inst, "symbolType");
            if (sType == "graphic")
            {
                el.isGraphic  = true;
                el.firstFrame = safeInt(inst, "firstFrame", 0);
                std::string loop = safeString(inst, "loop", "loop");
                el.looping    = (loop == "loop");
            }
        }
    }

    return el;
}

// =========================
// PARSE LAYERS
// =========================
static int parseLayers(const json& timelineNode, TA_Timeline& out)
{
    int maxFrame = 0;
    if (!timelineNode.contains("LAYERS")) return 1;

    for (auto& layer : timelineNode["LAYERS"])
    {
        TA_Layer l;
        if (!layer.contains("Frames")) continue;

        for (auto& frame : layer["Frames"])
        {
            TA_Frame f;
            f.index    = safeInt(frame, "index");
            f.duration = safeInt(frame, "duration", 1);

            int end = f.index + f.duration;
            if (end > maxFrame) maxFrame = end;

            if (frame.contains("elements") && frame["elements"].is_array())
                for (auto& e : frame["elements"])
                    f.elements.push_back(parseElement(e));

            l.frames.push_back(f);
        }
        out.layers.push_back(l);
    }

    return maxFrame > 0 ? maxFrame : 1;
}

// =========================
// LOAD
// =========================
bool TimelineAnimator::load(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    json j;
    try { file >> j; }
    catch (...) { return false; }

    if (!j.contains("SYMBOL_DICTIONARY")) return false;
    if (!j["SYMBOL_DICTIONARY"].contains("Symbols")) return false;

    for (auto& sym : j["SYMBOL_DICTIONARY"]["Symbols"])
    {
        std::string symName = safeString(sym, "SYMBOL_name");
        if (symName.empty()) continue;

        TA_Timeline t;
        if (sym.contains("TIMELINE") && !sym["TIMELINE"].is_null())
            t.totalFrames = parseLayers(sym["TIMELINE"], t);
        else
            t.totalFrames = 1;

        symbols[symName] = std::move(t);
    }

    // Bug 1 fix: the root ANIMATION object lives outside SYMBOL_DICTIONARY
    // and would otherwise be silently ignored. Register it in symbols so
    // play() can find it by its SYMBOL_name.
    if (j.contains("ANIMATION") && !j["ANIMATION"].is_null())
    {
        auto& anim = j["ANIMATION"];
        std::string animName = safeString(anim, "SYMBOL_name");
        if (!animName.empty() && !symbols.count(animName))
        {
            TA_Timeline t;
            if (anim.contains("TIMELINE") && !anim["TIMELINE"].is_null())
                t.totalFrames = parseLayers(anim["TIMELINE"], t);
            else
                t.totalFrames = 1;
            symbols[animName] = std::move(t);
        }
    }

    for (auto it = symbols.begin(); it != symbols.end(); ++it)
    {
        if (it->first.find("_ANIM_") != std::string::npos)
        {
            activeTimeline = &it->second;
            totalFrames    = activeTimeline->totalFrames;
            break;
        }
    }

    return true;
}

// =========================
// PLAY
// =========================
void TimelineAnimator::play(const std::string& entity, const std::string& animType)
{
    std::string key = entity + "_ANIM_" + animType;

    auto it = symbols.find(key);
    if (it == symbols.end()) return;

    activeTimeline = &it->second;
    totalFrames    = activeTimeline->totalFrames;
    currentFrame   = 0;
    elapsedTime    = 0.0f;
}

// =========================
// UPDATE
// =========================
void TimelineAnimator::update(float dt)
{
    if (!activeTimeline) return;

    elapsedTime += dt;

    // Keep currentFrame in sync for external readers (e.g. game logic checks).
    int totalF = totalFrames > 0 ? totalFrames : 1;
    currentFrame = (int)(elapsedTime * fps) % totalF;
}

// =========================
// DRAW ENTRY — applies parent transform before descending
// =========================
void TimelineAnimator::draw(Image* img, Atlas* atlas, Camera& cam)
{
    if (!activeTimeline) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1, 1, 1, 1);

    // If a parent transform is set, use it as the root origin.
    // Otherwise start at world origin (0,0) with identity rotation/scale.
    Vec2  rootPos   = parent.enabled ? parent.position : Vec2{0, 0};
    float rootRot   = parent.enabled ? parent.rotation : 0.0f;
    Vec2  rootScale = parent.enabled ? parent.scale    : Vec2{1, 1};

    // Pass elapsed time so every nested symbol derives its own frame
    // independently at the shared fps, instead of inheriting the parent's
    // raw frame index (which made shorter symbols cycle proportionally faster).
    drawTimeline(*activeTimeline, img, atlas, rootPos, rootRot, rootScale, elapsedTime);

    glDisable(GL_BLEND);
}

// =========================
// DRAW SPRITE HELPER
// =========================
void TimelineAnimator::drawSprite(
    const std::string& name,
    Image* img, Atlas* atlas,
    Vec2 pos, float rotRad, Vec2 scale, Vec2 pivot,
    Vec2 bitmapOff
)
{
    Frame fr;
    if (!atlas->get(name, fr)) return;

    float rotDeg = rotRad * (180.0f / 3.14159265f);

    float w = fr.w * scale.x;
    float h = fr.h * scale.y;

    float px = pivot.x * scale.x;
    float py = pivot.y * scale.y;

    float bx = bitmapOff.x * scale.x;
    float by = bitmapOff.y * scale.y;

    float u1 = fr.x / (float)img->width;
    float v1 = fr.y / (float)img->height;
    float u2 = (fr.x + fr.w) / (float)img->width;
    float v2 = (fr.y + fr.h) / (float)img->height;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, img->textureID);

    glPushMatrix();

    glTranslatef(pos.x, pos.y, 0);

    // Bug 3 fix: bitmapOff is a pre-rotation local offset (the sprite-sheet
    // frame's registration point within the symbol).  It must be translated
    // *before* the pivot/rotate so it rotates with the sprite.
    glTranslatef(bx, by, 0);

    glTranslatef(px, py, 0);
    glRotatef(rotDeg, 0, 0, 1);
    glTranslatef(-px, -py, 0);

    glBegin(GL_QUADS);
    glTexCoord2f(u1, v1); glVertex2f(0, 0);
    glTexCoord2f(u2, v1); glVertex2f(w, 0);
    glTexCoord2f(u2, v2); glVertex2f(w, h);
    glTexCoord2f(u1, v2); glVertex2f(0, h);
    glEnd();

    glPopMatrix();
}

// =========================
// CORE RENDER
// =========================
void TimelineAnimator::drawTimeline(
    TA_Timeline& timeline,
    Image*       img,
    Atlas*       atlas,
    Vec2         parentPos,
    float        parentRot,
    Vec2         parentScale,
    float        elapsed       // seconds since this timeline started
)
{
    // Each symbol derives its current frame from elapsed time at the shared fps.
    // This ensures all symbols — regardless of their individual totalFrames —
    // advance at the same wall-clock rate instead of cycling proportionally faster
    // when their frame count is shorter than the root's.
    int safeTF = timeline.totalFrames > 0 ? timeline.totalFrames : 1;
    int frame  = (int)(elapsed * fps) % safeTF;

    for (int li = (int)timeline.layers.size() - 1; li >= 0; li--)
    {
        auto& layer = timeline.layers[li];
        for (auto& f : layer.frames)
        {
            if (frame < f.index || frame >= f.index + f.duration)
                continue;

            for (auto& e : f.elements)
            {
                float cosR = cosf(parentRot);
                float sinR = sinf(parentRot);
                float lx   = e.position.x * parentScale.x;
                float ly   = e.position.y * parentScale.y;

                Vec2 pos = {
                    parentPos.x + cosR * lx - sinR * ly,
                    parentPos.y + sinR * lx + cosR * ly
                };

                float rot  = parentRot + e.rotation;
                Vec2 scale = { parentScale.x * e.scale.x, parentScale.y * e.scale.y };

                if (!e.spriteName.empty())
                {
                    drawSprite(e.spriteName, img, atlas,
                        pos, rot, scale, e.pivot, e.bitmapOff);
                }

                if (!e.symbolName.empty() && symbols.count(e.symbolName))
                {
                    auto& sym = symbols[e.symbolName];
                    int symSafeTF = sym.totalFrames > 0 ? sym.totalFrames : 1;

                    float symElapsed;
                    if (e.isGraphic)
                    {
                        // Graphic: locked to the parent clock, offset by firstFrame.
                        // Convert firstFrame back to seconds so the offset is in time-space.
                        float firstFrameSecs = e.firstFrame / fps;
                        if (e.looping)
                            // Wrap within the symbol's own duration
                            symElapsed = fmodf(elapsed + firstFrameSecs,
                                               symSafeTF / fps);
                        else
                            // Play-once: clamp at the last frame
                            symElapsed = std::min(elapsed + firstFrameSecs,
                                                  (symSafeTF - 1) / fps);
                    }
                    else
                    {
                        // Movieclip: independent clock, starts from 0 when
                        // its parent timeline started. Same elapsed time, own length.
                        symElapsed = elapsed;
                    }

                    drawTimeline(sym, img, atlas, pos, rot, scale, symElapsed);
                }
            }
        }
    }
}
