// PlaylabsGL.cpp — compile alongside your other .cpp sources
#include "AbsolutEngine.h"
#include <GL/gl.h>
#include <windows.h>

extern "C"
{

// =============================================================
// Image
// =============================================================

Image* Playlabs_LoadImage(const char* path)
{
    if (!path) return nullptr;
    Image* img = new Image();
    if (!img->load(path)) { delete img; return nullptr; }
    return img;
}

void Playlabs_FreeImage(Image* img) { delete img; }

// =============================================================
// Atlas
// =============================================================

Atlas* Playlabs_LoadAtlas(const char* path)
{
    if (!path) return nullptr;
    Atlas* a = new Atlas();
    if (!a->load(path)) { delete a; return nullptr; }
    return a;
}

void Playlabs_FreeAtlas(Atlas* atlas) { delete atlas; }

// =============================================================
// Sound
// =============================================================

Sound* Playlabs_CreateSound()          { return new Sound(); }
void   Playlabs_DestroySound(Sound* s) { delete s; }

// =============================================================
// Sprite
// =============================================================

Sprite* Playlabs_CreateSprite()           { return new Sprite(); }
void    Playlabs_DestroySprite(Sprite* s) { delete s; }

// =============================================================
// Animator
// =============================================================

Animator* Playlabs_CreateAnimator()             { return new Animator(); }
void      Playlabs_DestroyAnimator(Animator* a) { delete a; }

// Called by the Playlabs_Anim(anim, ENTITY, CLIP) macro.
// entity + clip are already stringified by the macro's # operator.
// Internally calls anim->Play(entity, clip) which builds the key
// entity + "_ANIM_" + clip and looks it up in the symbol table.
void Playlabs_AnimPlay(
    Animator* anim,
    const char* entity,
    const char* clip
)
{
    if (!anim || !entity || !clip) return;
    anim->Play(entity, clip);
}

void Playlabs_SetAnimatorParent(
    Animator* anim,
    float x, float y,
    float rotationRadians,
    float scaleX, float scaleY
)
{
    if (!anim) return;
    anim->Parent.Enabled  = true;
    anim->Parent.Position = {x, y};
    anim->Parent.Rotation = rotationRadians;
    anim->Parent.Scale    = {scaleX, scaleY};
}

void Playlabs_ClearAnimatorParent(Animator* anim)
{
    if (!anim) return;
    anim->Parent.Enabled  = false;
    anim->Parent.Position = {0, 0};
    anim->Parent.Rotation = 0.0f;
    anim->Parent.Scale    = {1, 1};
}

void Playlabs_TickAnimator(
    Animator* anim,
    float dt,
    Image* img, Atlas* atlas, Camera* cam
)
{
    if (!anim || !img || !atlas || !cam) return;
    anim->Update(dt);
    anim->Draw(img, atlas, *cam);
}

// =============================================================
// Camera
// =============================================================

void Playlabs_ApplyCamera(Camera* cam, int sw, int sh)
{
    if (!cam) return;
    cam->apply(sw, sh);
}

// =============================================================
// Window
// =============================================================

void Playlabs_Present(Window* win)
{
    if (!win) return;
    SwapBuffers(win->getHDC());
}

void Playlabs_Clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// =============================================================
// Input
// =============================================================

void Playlabs_PollInput(Window* win)
{
    if (!win) return;
    Input::update(win->getHWND());
}

int  Playlabs_KeyDown(int vkey)    { return Input::isKeyDown(vkey)    ? 1 : 0; }
int  Playlabs_KeyPressed(int vkey) { return Input::isKeyPressed(vkey) ? 1 : 0; }

void Playlabs_MousePos(int* x, int* y)
{
    if (x) *x = Input::mouseX;
    if (y) *y = Input::mouseY;
}

// =============================================================
// AABB
// =============================================================

int Playlabs_AABBIntersects(
    float ax, float ay, float aw, float ah,
    float bx, float by, float bw, float bh
)
{
    return AABB(ax, ay, aw, ah).intersects(AABB(bx, by, bw, bh)) ? 1 : 0;
}

int Playlabs_AABBContains(
    float bx, float by, float bw, float bh,
    float px, float py
)
{
    return AABB(bx, by, bw, bh).contains(px, py) ? 1 : 0;
}

} // extern "C"
