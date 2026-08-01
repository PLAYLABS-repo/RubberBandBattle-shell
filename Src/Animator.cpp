#include "Animator.h"
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
static float SafeFloat(const json& j, const char* key, float fallback = 0.0f)
{
    if (!j.contains(key) || j[key].is_null()) return fallback;
    return j[key].get<float>();
}

static int SafeInt(const json& j, const char* key, int fallback = 0)
{
    if (!j.contains(key) || j[key].is_null()) return fallback;
    return j[key].get<int>();
}

static std::string SafeString(const json& j, const char* key, const std::string& fallback = "")
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
static AnimElement ParseElement(const json& e)
{
    AnimElement el;

    if (e.contains("ATLAS_SPRITE_instance") && !e["ATLAS_SPRITE_instance"].is_null())
    {
        auto& sp = e["ATLAS_SPRITE_instance"];
        el.SpriteName = SafeString(sp, "name");

        if (sp.contains("Position") && !sp["Position"].is_null())
        {
            el.Position.x = SafeFloat(sp["Position"], "x");
            el.Position.y = SafeFloat(sp["Position"], "y");
        }
        if (sp.contains("transformationPoint") && !sp["transformationPoint"].is_null())
        {
            el.Pivot.x = SafeFloat(sp["transformationPoint"], "x");
            el.Pivot.y = SafeFloat(sp["transformationPoint"], "y");
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
                el.Position.x = SafeFloat(dm["Position"], "x");
                el.Position.y = SafeFloat(dm["Position"], "y");
            }
            if (dm.contains("Scaling") && !dm["Scaling"].is_null())
            {
                el.Scale.x = SafeFloat(dm["Scaling"], "x", 1.0f);
                el.Scale.y = SafeFloat(dm["Scaling"], "y", 1.0f);
            }
            if (dm.contains("Rotation") && !dm["Rotation"].is_null())
            {
                el.Rotation = SafeFloat(dm["Rotation"], "z");
            }
        }

        if (inst.contains("transformationPoint") && !inst["transformationPoint"].is_null())
        {
            el.Pivot.x = SafeFloat(inst["transformationPoint"], "x");
            el.Pivot.y = SafeFloat(inst["transformationPoint"], "y");
        }

        if (inst.contains("bitmap") && !inst["bitmap"].is_null())
        {
            auto& bm = inst["bitmap"];
            el.SpriteName  = SafeString(bm, "name");
            el.BitmapOff.x = SafeFloat(bm.contains("Position") ? bm["Position"] : json{}, "x");
            el.BitmapOff.y = SafeFloat(bm.contains("Position") ? bm["Position"] : json{}, "y");
        }
        else
        {
            el.SymbolName = SafeString(inst, "SYMBOL_name");

            std::string sType = SafeString(inst, "symbolType");
            if (sType == "graphic")
            {
                el.IsGraphic  = true;
                el.FirstFrame = SafeInt(inst, "firstFrame", 0);
                std::string loop = SafeString(inst, "loop", "loop");
                el.Looping    = (loop == "loop");
            }
        }
    }

    return el;
}

// =========================
// PARSE LAYERS
// =========================
static int ParseLayers(const json& animNode, AnimTimeline& out)
{
    int maxFrame = 0;
    if (!animNode.contains("LAYERS")) return 1;

    for (auto& layer : animNode["LAYERS"])
    {
        AnimLayer l;
        if (!layer.contains("Frames")) continue;

        for (auto& frame : layer["Frames"])
        {
            AnimFrame f;
            f.Index    = SafeInt(frame, "index");
            f.Duration = SafeInt(frame, "duration", 1);

            int end = f.Index + f.Duration;
            if (end > maxFrame) maxFrame = end;

            if (frame.contains("elements") && frame["elements"].is_array())
                for (auto& e : frame["elements"])
                    f.Elements.push_back(ParseElement(e));

            l.Frames.push_back(f);
        }
        out.Layers.push_back(l);
    }

    return maxFrame > 0 ? maxFrame : 1;
}

// =========================
// LOAD
// =========================
bool Animator::Load(const char* path)
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
        std::string symName = SafeString(sym, "SYMBOL_name");
        if (symName.empty()) continue;

        AnimTimeline t;
        if (sym.contains("TIMELINE") && !sym["TIMELINE"].is_null())
            t.TotalFrames = ParseLayers(sym["TIMELINE"], t);
        else
            t.TotalFrames = 1;

        Symbols[symName] = std::move(t);
    }

    for (auto it = Symbols.begin(); it != Symbols.end(); ++it)
    {
        if (it->first.find("_ANIM_") != std::string::npos)
        {
            ActiveAnim  = &it->second;
            TotalFrames = ActiveAnim->TotalFrames;
            break;
        }
    }

    return true;
}

// =========================
// PLAY
// =========================
void Animator::Play(const std::string& entity, const std::string& animType)
{
    std::string key = entity + "_ANIM_" + animType;

    auto it = Symbols.find(key);
    if (it == Symbols.end()) return;

    ActiveAnim   = &it->second;
    TotalFrames  = ActiveAnim->TotalFrames;
    CurrentFrame = 0;
    FrameTimer   = 0.0f;
}

// =========================
// CHANGE PART
// See Animator.h for the ChangePart vs ChangeParts distinction.
// =========================
void Animator::ChangePart(const std::string& oldSprite,
                           const std::string& newSprite,
                           const std::string& animKey)
{
    auto it = Symbols.find(animKey);
    if (it == Symbols.end()) return;

    SwapSpriteInAnim(it->second, oldSprite, newSprite, /*recursive=*/false);
}

void Animator::ChangeParts(const std::string& oldSprite,
                            const std::string& newSprite,
                            const std::string& animKey)
{
    auto it = Symbols.find(animKey);
    if (it == Symbols.end()) return;

    SwapSpriteInAnim(it->second, oldSprite, newSprite, /*recursive=*/true);
}

void Animator::SwapSpriteInAnim(AnimTimeline&       timeline,
                                 const std::string& oldSprite,
                                 const std::string& newSprite,
                                 bool                recursive)
{
    for (auto& layer : timeline.Layers)
        for (auto& frame : layer.Frames)
            for (auto& el : frame.Elements)
                SwapSpriteInElement(el, oldSprite, newSprite, recursive);
}

void Animator::SwapSpriteInElement(AnimElement&        el,
                                    const std::string& oldSprite,
                                    const std::string& newSprite,
                                    bool                recursive)
{
    if (el.SpriteName == oldSprite)
        el.SpriteName = newSprite;

    if (recursive && !el.SymbolName.empty())
    {
        auto it = Symbols.find(el.SymbolName);
        if (it != Symbols.end())
            SwapSpriteInAnim(it->second, oldSprite, newSprite, recursive);
    }
}

// =========================
// UPDATE
// =========================
void Animator::Update(float dt)
{
    if (!ActiveAnim) return;

    FrameTimer += dt;
    if (FrameTimer >= 1.0f / Fps)
    {
        FrameTimer = 0.0f;
        CurrentFrame++;
        if (CurrentFrame >= TotalFrames)
            CurrentFrame = 0;
    }
}

// =========================
// DRAW ENTRY — applies parent transform before descending
// =========================
void Animator::Draw(Image* img, Atlas* atlas, Camera& cam)
{
    if (!ActiveAnim) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1, 1, 1, 1);

    // If a parent transform is set, use it as the root origin.
    // Otherwise start at world origin (0,0) with identity rotation/scale.
    Vec2  rootPos   = Parent.Enabled ? Parent.Position : Vec2{0, 0};
    float rootRot   = Parent.Enabled ? Parent.Rotation : 0.0f;
    Vec2  rootScale = Parent.Enabled ? Parent.Scale    : Vec2{1, 1};

    DrawAnim(*ActiveAnim, img, atlas, rootPos, rootRot, rootScale, CurrentFrame);

    glDisable(GL_BLEND);
}

// =========================
// DRAW SPRITE HELPER
// =========================
void Animator::DrawSprite(
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

    glTranslatef(px, py, 0);
    glRotatef(rotDeg, 0, 0, 1);
    glTranslatef(-px, -py, 0);

    glTranslatef(bx, by, 0);

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
void Animator::DrawAnim(
    AnimTimeline& timeline,
    Image*        img,
    Atlas*        atlas,
    Vec2          parentPos,
    float         parentRot,
    Vec2          parentScale,
    int           frame
)
{
    for (int li = (int)timeline.Layers.size() - 1; li >= 0; li--)
    {
        auto& layer = timeline.Layers[li];
        for (auto& f : layer.Frames)
        {
            if (frame < f.Index || frame >= f.Index + f.Duration)
                continue;

            for (auto& e : f.Elements)
            {
                float cosR = cosf(parentRot);
                float sinR = sinf(parentRot);
                float lx   = e.Position.x * parentScale.x;
                float ly   = e.Position.y * parentScale.y;

                Vec2 pos = {
                    parentPos.x + cosR * lx - sinR * ly,
                    parentPos.y + sinR * lx + cosR * ly
                };

                float rot  = parentRot + e.Rotation;
                Vec2 scale = { parentScale.x * e.Scale.x, parentScale.y * e.Scale.y };

                if (!e.SpriteName.empty())
                {
                    DrawSprite(e.SpriteName, img, atlas,
                        pos, rot, scale, e.Pivot, e.BitmapOff);
                }

                if (!e.SymbolName.empty() && Symbols.count(e.SymbolName))
                {
                    auto& sym = Symbols[e.SymbolName];

                    int symFrame;
                    if (e.IsGraphic)
                        symFrame = e.FirstFrame % (sym.TotalFrames > 0 ? sym.TotalFrames : 1);
                    else
                        symFrame = frame % (sym.TotalFrames > 0 ? sym.TotalFrames : 1);

                    DrawAnim(sym, img, atlas, pos, rot, scale, symFrame);
                }
            }
        }
    }
}
