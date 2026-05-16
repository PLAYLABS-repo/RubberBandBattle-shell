
#include "PlaylabsGL.h"
#include <GL/gl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <iostream>
#include <cstdint>
#include <inttypes.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "include/stb_truetype.h"

// Build 0.0.14
// =============================================================
// CONSTANTS
// =============================================================
static const float   PI             = 3.14159265f;
static const float   BULLET_GRAVITY = 600.0f;
static const float   MIN_POWER      = 200.0f;
static const float   MAX_POWER      = 900.0f;
static const float   SHOOT_COOLDOWN = 0.15f;
static const float   BULLET_LIFETIME= 4.0f;
static const float   ARROW_SIZE     = 12.0f;
static const int     ARC_SEGMENTS   = 40;
static const float   ARC_STEP_T     = 0.08f;
static const int64_t MAX_AMMO       = 25;

// Punch constants
static const float PUNCH_RANGE    = 80.0f;
static const float PUNCH_DURATION = 0.25f;
static const float PUNCH_COOLDOWN = 0.45f;

// Loading bar constants
static const float LOAD_BAR_W     = 480.0f;
static const float LOAD_BAR_H     = 32.0f;
static const float LOAD_FILL_RATE = 0.55f;

// HP / damage constants
static const int PLAYER_MAX_HP = 100;
static const int BOT_MAX_HP    = 100;
static const int BULLET_DAMAGE = 10;
static const int PUNCH_DAMAGE  = 25;

// Floating HP bar dimensions (world-space units)
static const float WORLD_HP_BAR_W  = 80.0f;
static const float WORLD_HP_BAR_H  = 8.0f;
static const float WORLD_HP_BAR_OY = -24.0f;   // offset above sprite top
static const float WORLD_LABEL_OY  = -38.0f;   // offset for username text

// =============================================================
// FONT
// =============================================================
struct FontRenderer
{
    stbtt_bakedchar cdata[96];
    GLuint          texID = 0;
    float           size  = 0.0f;

    bool load(const char* path, float pixelHeight)
    {
        FILE* f = fopen(path, "rb");
        if (!f) return false;
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);
        unsigned char* ttf = new unsigned char[len];
        fread(ttf, 1, len, f);
        fclose(f);

        const int BW = 512, BH = 512;
        unsigned char* bitmap = new unsigned char[BW * BH];
        stbtt_BakeFontBitmap(ttf, 0, pixelHeight, bitmap, BW, BH, 32, 96, cdata);
        delete[] ttf;

        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, BW, BH, 0,
                     GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        delete[] bitmap;

        size = pixelHeight;
        return true;
    }

    void draw(const char* text, float x, float y,
              float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f)
    {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, texID);
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        float cx = x, cy = y;
        for (const char* p = text; *p; ++p)
        {
            if (*p < 32 || *p > 127) continue;
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, *p - 32, &cx, &cy, &q, 1);
            glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
            glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
            glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
            glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
        }
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glColor4f(1, 1, 1, 1);
    }

    // Returns approximate pixel width of a string (based on baked char advances).
    float measureWidth(const char* text)
    {
        float cx = 0.0f, cy = 0.0f;
        for (const char* p = text; *p; ++p)
        {
            if (*p < 32 || *p > 127) continue;
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, *p - 32, &cx, &cy, &q, 1);
        }
        return cx;
    }

    void destroy() { if (texID) { glDeleteTextures(1, &texID); texID = 0; } }
};

// =============================================================
// HELPERS
// =============================================================
static void drawRect(float x, float y, float w, float h,
                     float r, float g, float b, float a = 1.0f)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

static void drawLine(float x0, float y0, float x1, float y1,
                     float r, float g, float b, float a = 1.0f, float width = 2.0f)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(width);
    glColor4f(r, g, b, a);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();
    glLineWidth(1.0f);
    glColor4f(1, 1, 1, 1);
}

static void drawTriangle(float ax, float ay, float bx, float by,
                          float cx, float cy,
                          float r, float g, float b, float a = 1.0f)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLES);
    glVertex2f(ax, ay);
    glVertex2f(bx, by);
    glVertex2f(cx, cy);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

static void drawImageStretched(Image* img, float x, float y,
                                float w, float h, float alpha = 1.0f)
{
    if (!img) return;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, img->textureID);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x,     y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x,     y + h);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1, 1, 1, 1);
}

static void drawProgressBar(Image* fillImg, Image* borderImg,
                             float x, float y, float w, float h,
                             float fill, float alpha = 1.0f)
{
    if (fillImg && fill > 0.0f)
    {
        float fw = w * fill;
        float u1 = fill;

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, fillImg->textureID);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x,      y);
        glTexCoord2f(u1,   0.0f); glVertex2f(x + fw, y);
        glTexCoord2f(u1,   1.0f); glVertex2f(x + fw, y + h);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x,      y + h);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glColor4f(1, 1, 1, 1);
    }

    drawImageStretched(borderImg, x, y, w, h, alpha);
}

// Reusable HUD HP bar helper (green → yellow → red, with label and numeric readout).
static void drawHpBar(FontRenderer& font,
                      const char* label, int hp, int maxHp,
                      float x, float y, float barW, float barH,
                      float labelR, float labelG, float labelB)
{
    float fill = (maxHp > 0) ? (float)hp / (float)maxHp : 0.0f;
    if (fill < 0.0f) fill = 0.0f;
    if (fill > 1.0f) fill = 1.0f;

    float hr = (fill > 0.5f) ? (1.0f - fill) * 2.0f : 1.0f;
    float hg = (fill > 0.5f) ? 1.0f               : fill * 2.0f;

    drawRect(x - 4.0f, y - 4.0f, barW + 8.0f, barH + 28.0f, 0.0f, 0.0f, 0.0f, 0.5f);
    font.draw(label, x, y + 12.0f, labelR, labelG, labelB, 1.0f);
    drawRect(x, y + 16.0f, barW, barH, 0.15f, 0.15f, 0.15f, 0.9f);
    drawRect(x, y + 16.0f, barW * fill, barH, hr, hg, 0.1f, 1.0f);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d / %d", hp, maxHp);
    font.draw(buf, x + barW + 8.0f, y + 30.0f, 1.0f, 1.0f, 1.0f, 0.9f);
}

// -------------------------------------------------------------
// World-space floating HP bar + username drawn above a sprite.
//   spriteX/Y : top-left of the sprite in world coords
//   spriteW   : sprite width (bar is centred on the sprite)
//   username  : name string rendered above the bar
//   hp/maxHp  : current and maximum health
//   nameR/G/B : colour of the username text
// -------------------------------------------------------------
static void drawWorldHpBar(FontRenderer& font,
                            float spriteX, float spriteY, float spriteW,
                            const char* username,
                            int hp, int maxHp,
                            float nameR, float nameG, float nameB)
{
    float fill = (maxHp > 0) ? (float)hp / (float)maxHp : 0.0f;
    if (fill < 0.0f) fill = 0.0f;
    if (fill > 1.0f) fill = 1.0f;

    float hr = (fill > 0.5f) ? (1.0f - fill) * 2.0f : 1.0f;
    float hg = (fill > 0.5f) ? 1.0f                 : fill * 2.0f;

    // Centre the bar horizontally over the sprite
    float cx  = spriteX + spriteW * 0.5f;
    float barX = cx - WORLD_HP_BAR_W * 0.5f;
    float barY = spriteY + WORLD_HP_BAR_OY;

    // Dark background behind bar + label
    float bgPad = 4.0f;
    float labelH = 14.0f;
    drawRect(barX - bgPad,
             barY - labelH - bgPad,
             WORLD_HP_BAR_W + bgPad * 2.0f,
             WORLD_HP_BAR_H + labelH + bgPad * 2.0f,
             0.0f, 0.0f, 0.0f, 0.55f);

    // Username above the bar
    // stb_truetype draws with y as baseline, so offset accordingly
    float nameY = barY - 2.0f;
    float nameW = font.measureWidth(username);
    float nameX = cx - nameW * 0.5f;
    font.draw(username, nameX, nameY, nameR, nameG, nameB, 1.0f);

    // Bar track
    drawRect(barX, barY, WORLD_HP_BAR_W, WORLD_HP_BAR_H, 0.12f, 0.12f, 0.12f, 0.9f);
    // Bar fill
    drawRect(barX, barY, WORLD_HP_BAR_W * fill, WORLD_HP_BAR_H, hr, hg, 0.1f, 1.0f);
}

// =============================================================
// CAMERA
// =============================================================
static void applyCamera2D(Camera& cam, int sw, int sh)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float visW = sw / cam.zoom;
    float visH = sh / cam.zoom;
    glOrtho(cam.position.x, cam.position.x + visW,
            cam.position.y + visH, cam.position.y,
            -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void applyScreenSpace(int sw, int sh)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, sw, sh, 0, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void screenToWorld(float sx, float sy,
                           const Camera& cam, int sw, int sh,
                           float& wx, float& wy)
{
    float visW = sw / cam.zoom;
    float visH = sh / cam.zoom;
    wx = cam.position.x + (sx / sw) * visW;
    wy = cam.position.y + (sy / sh) * visH;
}

// =============================================================
// BULLET
// =============================================================
struct Bullet
{
    float x, y;
    float velX, velY;
    float w    = 48.0f;
    float h    = 48.0f;
    float hitW = 40.0f;
    float hitH = 40.0f;
    float life = BULLET_LIFETIME;
    bool  dead = false;

    Sprite* sprite = nullptr;

    float hitX() const { return x + (w - hitW) * 0.5f; }
    float hitY() const { return y + (h - hitH) * 0.5f; }
};

// =============================================================
// PLAYER
// =============================================================
struct Player
{
    float x     = 200.0f;
    float y     = 436.0f;
    float w     = 64.0f;
    float h     = 64.0f;
    float speed = 250.0f;

    float minY = 100.0f;
    float maxY = 500.0f;

    float baseY     = 436.0f;
    float velocityY = 0.0f;
    float gravity   = 900.0f;
    float jumpForce = -520.0f;
    bool  jumping   = false;

    float chargeTimer   = 0.0f;
    float shootCooldown = 0.0f;
    int   ammo          = MAX_AMMO;

    // HP
    int hp    = PLAYER_MAX_HP;
    int maxHp = PLAYER_MAX_HP;

    // aimDir: fire direction — AWAY from mouse (slingshot/rubber-band style)
    float aimDirX = 1.0f;
    float aimDirY = 0.0f;

    // --- Punch state ---
    bool  punching      = false;
    float punchTimer    = 0.0f;
    float punchCooldown = 0.0f;
    bool  punchHit      = false;

    Sprite*           sprite = nullptr;
    TimelineAnimator* anim   = nullptr;

    enum class State { IDLE, RUN, JUMP, PUNCH } state = State::IDLE;
    float facingX = 1.0f;

    float muzzleX() const { return x + w * 0.5f; }
    float muzzleY() const { return y + h * 0.5f; }

    void getPunchHitbox(float& hx, float& hy, float& hw, float& hh) const
    {
        hw = PUNCH_RANGE;
        hh = 200.0f;
        hx = (facingX < 0.0f) ? x + w : x - hw;
        hy = y + h;
    }
};

// =============================================================
// BOT
// =============================================================
static const float BOT_SHOOT_COOLDOWN = 1.8f;
static const float BOT_PUNCH_DIST     = 120.0f;
static const float BOT_SHOOT_DIST     = 600.0f;
static const float BOT_REACTION_TIMER = 0.6f;
static const float BOT_AIM_NOISE      = 0.10f;

struct Bot
{
    float x     = 700.0f;
    float y     = 436.0f;
    float w     = 64.0f;
    float h     = 64.0f;
    float speed = 180.0f;

    float minY = 100.0f;
    float maxY = 500.0f;

    float baseY     = 436.0f;
    float velocityY = 0.0f;
    float gravity   = 900.0f;
    float jumpForce = -520.0f;
    bool  jumping   = false;

    float shootCooldown = BOT_SHOOT_COOLDOWN;
    float reactionTimer = 0.0f;
    int   ammo          = (int)MAX_AMMO;

    // HP
    int hp    = BOT_MAX_HP;
    int maxHp = BOT_MAX_HP;

    float aimDirX = -1.0f;
    float aimDirY =  0.0f;
    float facingX = -1.0f;

    bool  punching      = false;
    float punchTimer    = 0.0f;
    float punchCooldown = 0.0f;
    bool  punchHit      = false;

    Sprite*           sprite = nullptr;
    TimelineAnimator* anim   = nullptr;

    enum class State { IDLE, RUN, JUMP, PUNCH } state = State::IDLE;

    float muzzleX() const { return x + w * 0.5f; }
    float muzzleY() const { return y + h * 0.5f; }

    void getPunchHitbox(float& hx, float& hy, float& hw, float& hh) const
    {
        hw = PUNCH_RANGE;
        hh = 200.0f;
        hx = (facingX < 0.0f) ? x + w : x - hw;
        hy = y + h;
    }
};

// =============================================================
// ARC PREVIEW
// =============================================================
static void drawArcPreview(float ox, float oy,
                            float vx, float vy,
                            float r, float g, float b)
{
    float px = ox, py = oy;
    float pvx = vx, pvy = vy;
    float endX = ox, endY = oy;
    float prevX = ox, prevY = oy;

    for (int i = 0; i < ARC_SEGMENTS; ++i)
    {
        float nx  = px + pvx * ARC_STEP_T;
        float ny  = py + pvy * ARC_STEP_T + 0.5f * BULLET_GRAVITY * ARC_STEP_T * ARC_STEP_T;
        float nvy = pvy + BULLET_GRAVITY * ARC_STEP_T;

        float alpha = 1.0f - (float)i / ARC_SEGMENTS;
        drawLine(px, py, nx, ny, r, g, b, alpha, 2.5f);

        prevX = px; prevY = py;
        endX  = nx; endY  = ny;
        px  = nx; py  = ny;
        pvy = nvy;
    }

    float dx = endX - prevX;
    float dy = endY - prevY;
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.0001f)
    {
        dx /= len;
        dy /= len;

        float tx  = endX + dx * ARROW_SIZE;
        float ty  = endY + dy * ARROW_SIZE;
        float px1 = endX - dy * ARROW_SIZE * 0.5f;
        float py1 = endY + dx * ARROW_SIZE * 0.5f;
        float px2 = endX + dy * ARROW_SIZE * 0.5f;
        float py2 = endY - dx * ARROW_SIZE * 0.5f;

        drawTriangle(tx, ty, px1, py1, px2, py2, r, g, b, 1.0f);
    }
}

// =============================================================
// ENTRY POINT
// =============================================================
int main()
{
    Window win;
    if (!win.create("RubberBandBattle-Shell", 1280, 720))
        return -1;

    Camera cam;
    cam.position = {0.0f, 0.0f};
    cam.zoom     = 1.723f;

    Timer timer;

    float fpsTimer   = 0.0f;
    int   fpsFrames  = 0;
    float currentFPS = 0.0f;

    FontRenderer font;
    font.load("Resources/Font/Confale.ttf", 28.0f);

    // Small font for world-space labels (scaled down relative to world units)
    FontRenderer worldFont;
    worldFont.load("Resources/Font/Confale.ttf", 14.0f);

    // ---------------------------------------------------------
    // Loading screen
    // ---------------------------------------------------------
    enum class GameState { LOADING, PLAYING };
    GameState gameState = GameState::LOADING;

    Image* loadingImage      = PL_LoadImage("loading.png");
    Image* progressBorderImg = PL_LoadImage("progress_border.png");
    Image* progressBarImg    = PL_LoadImage("progress_bar.png");

    Sound* loadingMusic = CreateSound();
    loadingMusic->load("loading.wav");
    loadingMusic->play(true);

    float loadingFade      = 1.0f;
    bool  loadingFadingOut = false;
    const float FADE_SPEED = 2.5f;

    float promptPulse    = 1.0f;
    bool  prevAnyKeyDown = false;

    float loadProgress = 0.0f;
    bool  loadComplete = false;

    // ---------------------------------------------------------
    // Game assets
    // ---------------------------------------------------------
    Image* sheet = PL_LoadImage("Resources/Skins/spritemap.png");
    Atlas* atlas = LoadAtlas("Resources/Skins/spritemap.json");

    Sound* bgm = CreateSound();
    bgm->load("loading.wav");

    Sound* sfxJump = CreateSound();
    sfxJump->load("jump.wav");

    Sound* sfxPunch = CreateSound();
    sfxPunch->load("punch.wav");

    // ---------------------------------------------------------
    // Player
    // ---------------------------------------------------------
    Player player;

    player.sprite                  = CreateSprite();
    player.sprite->image           = sheet;
    player.sprite->atlas           = atlas;
    player.sprite->frameName       = "0010";
    player.sprite->position        = {player.x, player.y};
    player.sprite->targetPosition  = {player.x, player.y};
    player.sprite->scale           = {1.0f, 1.0f};
    player.sprite->targetScale     = {1.0f, 1.0f};

    player.anim = CreateAnimator();
    if (player.anim->load("Resources/Skins/Animation.json"))
        Anim(player.anim, PLAYER, IDLE);

    // ---------------------------------------------------------
    // Bot
    // ---------------------------------------------------------
    Bot bot;

    bot.sprite                 = CreateSprite();
    bot.sprite->image          = sheet;
    bot.sprite->atlas          = atlas;
    bot.sprite->frameName      = "0010";
    bot.sprite->position       = {bot.x, bot.y};
    bot.sprite->targetPosition = {bot.x, bot.y};
    bot.sprite->scale          = {bot.facingX, 1.0f};
    bot.sprite->targetScale    = {bot.facingX, 1.0f};

    bot.anim = CreateAnimator();
    if (bot.anim->load("Resources/Skins/Animation.json"))
        Anim(bot.anim, PLAYER, IDLE);

    // ---------------------------------------------------------
    // Background
    // ---------------------------------------------------------
    Sprite* background         = CreateSprite();
    background->image          = sheet;
    background->atlas          = atlas;
    background->frameName      = "0000";
    background->position       = {0.0f, 0.0f};
    background->targetPosition = {0.0f, 0.0f};

    // ---------------------------------------------------------
    // Bullets
    // ---------------------------------------------------------
    std::vector<Bullet> bullets;
    bullets.reserve(64);

    bool prevMouseHeld  = false;
    bool prevRMouseHeld = false;

    // ==========================================================
    // GAME LOOP
    // ==========================================================
    while (win.process())
    {
        Sleep(10);
        float dt = timer.delta();
        if (dt > 0.05f) dt = 0.05f;

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 1.0f)
        {
            currentFPS = fpsFrames / fpsTimer;
            fpsFrames  = 0;
            fpsTimer   = 0.0f;
        }

        PollInput(&win);

        int sw = win.getWidth();
        int sh = win.getHeight();

        // ======================================================
        // LOADING SCREEN
        // ======================================================
        if (gameState == GameState::LOADING)
        {
            promptPulse += dt;

            if (!loadComplete)
            {
                loadProgress += LOAD_FILL_RATE * dt;
                if (loadProgress >= 1.0f)
                {
                    loadProgress = 1.0f;
                    loadComplete = true;
                }
            }

            bool anyKeyDown = loadComplete &&
                              (KeyDown(VK_LBUTTON) ||
                               KeyDown(VK_RETURN)  ||
                               KeyDown(VK_SPACE)   ||
                               KeyDown(VK_ESCAPE));

            bool justPressed = anyKeyDown && !prevAnyKeyDown;
            prevAnyKeyDown   = anyKeyDown;

            if (justPressed && !loadingFadingOut)
                loadingFadingOut = true;

            if (loadingFadingOut)
            {
                loadingFade -= dt * FADE_SPEED;
                if (loadingFade <= 0.0f)
                {
                    loadingFade = 0.0f;
                    gameState   = GameState::PLAYING;
                    loadingMusic->stop();
                    bgm->play(true);
                }
            }

            PL_Clear(0.0f, 0.0f, 0.0f, 1.0f);
            applyScreenSpace(sw, sh);

            drawImageStretched(loadingImage, 0.0f, 0.0f, (float)sw, (float)sh, loadingFade);

            {
                float barX = (sw - LOAD_BAR_W) * 0.5f;
                float barY =  sh * 0.82f;

                drawProgressBar(progressBarImg, progressBorderImg,
                                barX, barY, LOAD_BAR_W, LOAD_BAR_H,
                                loadProgress, loadingFade);

                char pctBuf[16];
                snprintf(pctBuf, sizeof(pctBuf), "%d%%", (int)(loadProgress * 100.0f));
                float labelX = barX + LOAD_BAR_W * 0.5f - strlen(pctBuf) * 8.0f;
                float labelY = barY - 6.0f;
                font.draw(pctBuf, labelX, labelY, 1.0f, 1.0f, 1.0f, 0.85f * loadingFade);
            }

            if (!loadingFadingOut && loadComplete)
            {
                float pulse = sinf(promptPulse) * 0.5f + 0.5f;
                float hintA = 0.5f + pulse * 0.5f;

                const char* hint = "PRESS ANY KEY TO START";
                float hintX = sw * 0.5f - strlen(hint) * 8.0f;
                float hintY = sh * 0.5f - strlen(hint) * 4.0f;

                drawRect(hintX - 16.0f, hintY - 26.0f,
                         strlen(hint) * 16.0f + 32.0f, 38.0f,
                         0.0f, 0.0f, 0.0f, 0.55f * loadingFade);

                font.draw(hint, hintX, hintY, 1.0f, 1.0f, 1.0f, hintA * loadingFade);
            }

            PL_Present(&win);
            continue;
        }

        // ======================================================
        // GAME LOGIC
        // ======================================================

        int mousePixelX = 0, mousePixelY = 0;
        MousePos(&mousePixelX, &mousePixelY);

        float mouseWorldX, mouseWorldY;
        screenToWorld((float)mousePixelX, (float)mousePixelY,
                      cam, sw, sh, mouseWorldX, mouseWorldY);

        // Vector from muzzle to mouse (the "pull" direction)
        float pullDX  = mouseWorldX - player.muzzleX();
        float pullDY  = mouseWorldY - player.muzzleY();
        float pullLen = sqrtf(pullDX * pullDX + pullDY * pullDY);

        // SLINGSHOT / RUBBER-BAND: fire direction is OPPOSITE to the pull.
        // Pulling the mouse to the right fires to the left, etc.
        if (pullLen > 0.001f)
        {
            player.aimDirX = -pullDX / pullLen;   // negated → away from mouse
            player.aimDirY = -pullDY / pullLen;
        }

        // Face the fire direction
        player.facingX = (player.aimDirX >= 0.0f) ? 1.0f : -1.0f;

        player.shootCooldown -= dt;
        player.punchCooldown -= dt;

        // ------------------------------------------------------
        // LEFT MOUSE — shoot on click, power = mouse distance
        // ------------------------------------------------------
        bool mouseHeld   = KeyDown(VK_LBUTTON) != 0;
        bool justClicked = mouseHeld && !prevMouseHeld;
        prevMouseHeld    = mouseHeld;

        if (player.ammo > 0 && justClicked && player.shootCooldown <= 0.0f)
        {
            float power = pullLen;
            player.chargeTimer = std::min(power / MAX_POWER, 1.0f);

            Bullet b;
            b.x    = player.muzzleX() - 24.0f;
            b.y    = player.muzzleY() - 24.0f;
            b.velX = player.aimDirX * power;   // fires away from mouse
            b.velY = player.aimDirY * power;

            b.sprite                 = CreateSprite();
            b.sprite->image          = sheet;
            b.sprite->atlas          = atlas;
            b.sprite->frameName      = "0000";
            b.sprite->scale          = {player.facingX, 1.0f};
            b.sprite->targetScale    = {player.facingX, 1.0f};
            b.sprite->position       = {b.x, b.y};
            b.sprite->targetPosition = {b.x, b.y};

            bullets.push_back(b);
            --player.ammo;
            player.shootCooldown = SHOOT_COOLDOWN;

            Event("bullet_fired", &player, &bullets.back());
        }

        // ------------------------------------------------------
        // RIGHT MOUSE — punch
        // ------------------------------------------------------
        bool rMouseHeld    = KeyDown(VK_RBUTTON) != 0;
        bool rJustPressed  = rMouseHeld && !prevRMouseHeld;
        prevRMouseHeld     = rMouseHeld;

        if (rJustPressed && !player.punching &&
            player.punchCooldown <= 0.0f && !player.jumping)
        {
            player.punching   = true;
            player.punchTimer = 0.0f;
            player.punchHit   = false;
            player.state      = Player::State::PUNCH;

            Anim(player.anim, PLAYER, PUNCH);
            sfxPunch->play(false);

            Event("punch_started", &player);
        }

        if (player.punching)
        {
            player.punchTimer += dt;

            // Placeholder wall hitbox
            const float wX = 100.0f, wY = 100.0f, wW = 100.0f, wH = 100.0f;

            if (!player.punchHit)
            {
                float phx, phy, phw, phh;
                player.getPunchHitbox(phx, phy, phw, phh);

                // Check punch vs bot first
                {
                    const float bHitSzX = 100.0f;
                    float bHitX = bot.x + (bot.w - bHitSzX) * 0.5f;
                    float bHitY = bot.y + bot.h;
                    if (AABBIntersects(phx, phy, phw, phh, bHitX, bHitY, bHitSzX, 200.0f))
                    {
                        player.punchHit = true;
                        bot.hp -= PUNCH_DAMAGE;
                        if (bot.hp < 0) bot.hp = 0;
                        Event("player_punch_hit_bot", &player, &bot);
                    }
                }

                // Check punch vs wall (only if bot not already hit)
                if (!player.punchHit &&
                    AABBIntersects(phx, phy, phw, phh, wX, wY, wW, wH))
                {
                    player.punchHit = true;
                    Event("punch_hit", &player);
                }
            }

            if (player.punchTimer >= PUNCH_DURATION)
            {
                player.punching      = false;
                player.punchCooldown = PUNCH_COOLDOWN;
                player.state         = Player::State::IDLE;
                Anim(player.anim, PLAYER, IDLE);
            }
        }

        // ------------------------------------------------------
        // Movement
        // ------------------------------------------------------
        bool  moving = false;
        float moveX  = 0.0f;
        float moveY  = 0.0f;

        if (!player.punching)
        {
            if (KeyDown(VK_RIGHT) || KeyDown('D')) { moveX =  1.0f; moving = true; }
            if (KeyDown(VK_LEFT)  || KeyDown('A')) { moveX = -1.0f; moving = true; }

            if (!player.jumping)
            {
                if (KeyDown(VK_UP)   || KeyDown('W')) { moveY = -1.0f; moving = true; }
                if (KeyDown(VK_DOWN) || KeyDown('S')) { moveY =  1.0f; moving = true; }
            }
        }

        if (KeyPressed(VK_SPACE) && !player.jumping && !player.punching)
        {
            player.baseY     = player.y;
            player.velocityY = player.jumpForce;
            player.jumping   = true;
            player.state     = Player::State::JUMP;

            Anim(player.anim, PLAYER, JUMP);
            sfxJump->play(false);

            Event("player_jumped", &player);
        }

        player.x += moveX * player.speed * dt;

        if (!player.jumping)
        {
            player.y    += moveY * player.speed * dt;
            if (player.y < player.minY)              player.y = player.minY;
            if (player.y > player.maxY - player.h)  player.y = player.maxY - player.h;
            player.baseY = player.y;
        }
        else
        {
            player.velocityY += player.gravity * dt;
            player.y         += player.velocityY * dt;
            if (player.y >= player.baseY)
            {
                player.y         = player.baseY;
                player.velocityY = 0.0f;
                player.jumping   = false;

                if (!player.punching)
                {
                    player.state = moving ? Player::State::RUN : Player::State::IDLE;
                    if (moving) Anim(player.anim, PLAYER, RUN);
                    else        Anim(player.anim, PLAYER, IDLE);
                }

                Event("player_landed", &player);
            }
        }

        player.sprite->position       = {player.x, player.y};
        player.sprite->targetPosition = {player.x, player.y};
        player.sprite->scale          = {player.facingX, 1.0f};
        player.sprite->targetScale    = {player.facingX, 1.0f};

        if (!player.jumping && !player.punching)
        {
            if (moving && player.state != Player::State::RUN)
            {
                player.state = Player::State::RUN;
                Anim(player.anim, PLAYER, RUN);
            }
            else if (!moving && player.state != Player::State::IDLE)
            {
                player.state = Player::State::IDLE;
                Anim(player.anim, PLAYER, IDLE);
            }
        }

        SetAnimatorParent(player.anim, player.x, player.y, 0.0f, player.facingX, 1.0f);

        // Smooth camera follow
        float targetCamX = player.x - sw * 0.5f + 340.0f;
        float targetCamY = player.y - sh * 0.5f + 280.0f;
        cam.position.x  += (targetCamX - cam.position.x) * 5.0f * dt;
        cam.position.y  += (targetCamY - cam.position.y) * 5.0f * dt;

        // Placeholder wall
        const float wX = 100.0f, wY = 100.0f, wW = 100.0f, wH = 100.0f;

        // ------------------------------------------------------
        // Bullet update
        // ------------------------------------------------------
        for (Bullet& b : bullets)
        {
            if (b.dead) continue;

            b.velY += BULLET_GRAVITY * dt;
            b.x    += b.velX * dt;
            b.y    += b.velY * dt;

            b.sprite->position       = {b.x, b.y};
            b.sprite->targetPosition = {b.x, b.y};
            b.sprite->update(dt);

            b.life -= dt;
            if (b.life <= 0.0f) { b.dead = true; continue; }

            if (AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH,
                               wX, wY, wW, wH))
            {
                b.dead = true;
                Event("bullet_hit_wall", nullptr, &b);
            }

            // Bullet hits bot
            if (!b.dead)
            {
                const float bHitSzX = 100.0f, bHitSzY = 200.0f;
                float bHitX = bot.x + (bot.w - bHitSzX) * 0.5f;
                float bHitY = bot.y + bot.h;
                if (AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH,
                                   bHitX, bHitY, bHitSzX, bHitSzY))
                {
                    b.dead  = true;
                    bot.hp -= BULLET_DAMAGE;
                    if (bot.hp < 0) bot.hp = 0;
                    Event("bullet_hit_bot", nullptr, &b);
                }
            }

            // Bullet hits player
            if (!b.dead)
            {
                const float pHitSzX = 100.0f, pHitSzY = 200.0f;
                float pHitX = player.x + (player.w - pHitSzX) * 0.5f;
                float pHitY = player.y + player.h;
                if (AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH,
                                   pHitX, pHitY, pHitSzX, pHitSzY))
                {
                    b.dead     = true;
                    player.hp -= BULLET_DAMAGE;
                    if (player.hp < 0) player.hp = 0;
                    Event("bullet_hit_player", nullptr, &b);
                }
            }
        }

        // Cleanup dead bullets
        for (Bullet& b : bullets)
            if (b.dead && b.sprite) { DestroySprite(b.sprite); b.sprite = nullptr; }
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                           [](const Bullet& b){ return b.dead; }),
            bullets.end());

        // Player vs wall collision
        const float playerHitboxSizeX = 100.0f;
        const float playerHitboxSizeY = 200.0f;
        float hitW = player.jumping ? playerHitboxSizeX * 0.6f : playerHitboxSizeX;
        float hitH = player.jumping ? playerHitboxSizeY * 0.5f : playerHitboxSizeY;
        float hitX = player.x + (player.w - playerHitboxSizeX) * 0.5f;
        float hitY = player.y + player.h;

        if (AABBIntersects(hitX, hitY, hitW, hitH, wX, wY, wW, wH))
        {
            player.x -= moveX * player.speed * dt;
            player.sprite->position       = {player.x, player.y};
            player.sprite->targetPosition = {player.x, player.y};
        }

        // ==========================================================
        // BOT AI
        // ==========================================================
        {
            float distX = player.x - bot.x;
            float distY = player.y - bot.y;
            float dist  = sqrtf(distX * distX + distY * distY);

            bot.shootCooldown -= dt;
            bot.punchCooldown -= dt;
            bot.reactionTimer -= dt;

            // Face the player
            bot.facingX = (distX >= 0.0f) ? 1.0f : -1.0f;

            // Aim toward player with slight noise
            if (dist > 0.001f)
            {
                float noise     = ((float)(rand() % 100) / 100.0f - 0.5f) * BOT_AIM_NOISE;
                float baseAngle = atan2f(distY, distX) + noise;
                bot.aimDirX     = cosf(baseAngle);
                bot.aimDirY     = sinf(baseAngle);
            }

            bool botMoving = false;

            if (!bot.punching)
            {
                // Punch if player is very close
                if (dist < BOT_PUNCH_DIST && bot.punchCooldown <= 0.0f && !bot.jumping)
                {
                    bot.punching      = true;
                    bot.punchTimer    = 0.0f;
                    bot.punchHit      = false;
                    bot.state         = Bot::State::PUNCH;
                    Anim(bot.anim, PLAYER, PUNCH);
                }
                // Shoot if in range and cooled down
                else if (dist < BOT_SHOOT_DIST && bot.shootCooldown <= 0.0f && bot.ammo > 0
                         && bot.reactionTimer <= 0.0f)
                {
                    float power = std::min(dist * 1.1f, MAX_POWER);

                    Bullet b;
                    b.x    = bot.muzzleX() - 24.0f;
                    b.y    = bot.muzzleY() - 24.0f;
                    b.velX = bot.aimDirX * power;
                    b.velY = bot.aimDirY * power;

                    b.sprite                 = CreateSprite();
                    b.sprite->image          = sheet;
                    b.sprite->atlas          = atlas;
                    b.sprite->frameName      = "0000";
                    b.sprite->scale          = {bot.facingX, 1.0f};
                    b.sprite->targetScale    = {bot.facingX, 1.0f};
                    b.sprite->position       = {b.x, b.y};
                    b.sprite->targetPosition = {b.x, b.y};

                    bullets.push_back(b);
                    --bot.ammo;
                    bot.shootCooldown = BOT_SHOOT_COOLDOWN;
                    bot.reactionTimer = BOT_REACTION_TIMER;
                }
                // Chase the player horizontally
                else if (dist > BOT_PUNCH_DIST)
                {
                    float moveDir = (distX > 0.0f) ? 1.0f : -1.0f;
                    bot.x += moveDir * bot.speed * dt;
                    botMoving = true;
                }

                // Vertical chase (not while jumping)
                if (!bot.jumping)
                {
                    if (distY < -20.0f)      bot.y -= bot.speed * 0.5f * dt;
                    else if (distY > 20.0f)  bot.y += bot.speed * 0.5f * dt;

                    if (bot.y < bot.minY)             bot.y = bot.minY;
                    if (bot.y > bot.maxY - bot.h)     bot.y = bot.maxY - bot.h;
                    bot.baseY = bot.y;
                }
            }

            // Punch timer
            if (bot.punching)
            {
                bot.punchTimer += dt;

                if (!bot.punchHit)
                {
                    float phx, phy, phw, phh;
                    bot.getPunchHitbox(phx, phy, phw, phh);

                    const float playerHitboxSizeX = 100.0f;
                    float phbX = player.x + (player.w - playerHitboxSizeX) * 0.5f;
                    float phbY = player.y + player.h;
                    if (AABBIntersects(phx, phy, phw, phh, phbX, phbY, playerHitboxSizeX, 200.0f))
                    {
                        bot.punchHit  = true;
                        player.hp    -= PUNCH_DAMAGE;
                        if (player.hp < 0) player.hp = 0;
                        Event("bot_punch_hit", &bot, &player);
                    }
                }

                if (bot.punchTimer >= PUNCH_DURATION)
                {
                    bot.punching      = false;
                    bot.punchCooldown = PUNCH_COOLDOWN;
                    bot.state         = Bot::State::IDLE;
                    Anim(bot.anim, PLAYER, IDLE);
                }
            }

            // Jump physics
            if (bot.jumping)
            {
                bot.velocityY += bot.gravity * dt;
                bot.y         += bot.velocityY * dt;
                if (bot.y >= bot.baseY)
                {
                    bot.y         = bot.baseY;
                    bot.velocityY = 0.0f;
                    bot.jumping   = false;
                    bot.state     = botMoving ? Bot::State::RUN : Bot::State::IDLE;
                    if (botMoving) Anim(bot.anim, PLAYER, RUN);
                    else           Anim(bot.anim, PLAYER, IDLE);
                }
            }

            // Animation state
            if (!bot.jumping && !bot.punching)
            {
                if (botMoving && bot.state != Bot::State::RUN)
                {
                    bot.state = Bot::State::RUN;
                    Anim(bot.anim, PLAYER, RUN);
                }
                else if (!botMoving && bot.state != Bot::State::IDLE)
                {
                    bot.state = Bot::State::IDLE;
                    Anim(bot.anim, PLAYER, IDLE);
                }
            }

            bot.sprite->position       = {bot.x, bot.y};
            bot.sprite->targetPosition = {bot.x, bot.y};
            bot.sprite->scale          = {bot.facingX, 1.0f};
            bot.sprite->targetScale    = {bot.facingX, 1.0f};

            SetAnimatorParent(bot.anim, bot.x, bot.y, 0.0f, bot.facingX, 1.0f);
        }

        // ==========================================================
        // RENDER — world
        // ==========================================================
        PL_Clear(0.12f, 0.12f, 0.18f, 1.0f);
        applyCamera2D(cam, sw, sh);

        drawRect(0, player.maxY, 2000, 64, 0.25f, 0.20f, 0.15f);

        if (!sheet)
            drawRect(player.x, player.y, player.w, player.h, 0.2f, 0.6f, 1.0f);

        background->update(dt);
        background->draw(cam);

        for (Bullet& b : bullets)
        {
            b.sprite->draw(cam);
            drawRect(b.hitX(), b.hitY(), b.hitW, b.hitH, 1.0f, 1.0f, 0.0f, 0.35f);
        }

        player.sprite->update(dt);
        player.sprite->draw(cam);
        TickAnimator(player.anim, dt, sheet, atlas, &cam);

        // Bot render
        bot.sprite->update(dt);
        bot.sprite->draw(cam);
        TickAnimator(bot.anim, dt, sheet, atlas, &cam);

        // ----------------------------------------------------------
        // Floating world-space HP bars + usernames
        // Drawn while the camera projection is still active so they
        // sit exactly above each sprite in world coordinates.
        // ----------------------------------------------------------
        drawWorldHpBar(worldFont,
                       player.x, player.y, player.w,
                       "PLAYER",
                       player.hp, player.maxHp,
                       0.4f, 1.0f, 0.4f);   // green username

        drawWorldHpBar(worldFont,
                       bot.x, bot.y, bot.w,
                       "BOT",
                       bot.hp, bot.maxHp,
                       1.0f, 0.4f, 0.4f);   // red username

        // Debug: bot hitbox (orange) and punch box
        {
            const float bHitSzX = 100.0f, bHitSzY = 200.0f;
            float bHitX = bot.x + (bot.w - bHitSzX) * 0.5f;
            float bHitY = bot.y + bot.h;
            drawRect(bHitX, bHitY, bHitSzX, bHitSzY, 1.0f, 0.5f, 0.0f, 0.4f);

            if (bot.punching)
            {
                float phx, phy, phw, phh;
                bot.getPunchHitbox(phx, phy, phw, phh);
                drawRect(phx, phy, phw, phh,
                         bot.punchHit ? 1.0f : 1.0f,
                         bot.punchHit ? 0.2f : 0.6f,
                         bot.punchHit ? 0.2f : 0.0f,
                         bot.punchHit ? 0.9f : 0.55f);
            }
        }

        // Debug: wall and player hitbox
        drawRect(wX, wY, wW, wH, 1.0f, 0.0f, 0.0f, 0.5f);
        drawRect(hitX, hitY, hitW, hitH, 0.0f, 1.0f, 0.5f, 0.5f);

        if (player.punching)
        {
            float phx, phy, phw, phh;
            player.getPunchHitbox(phx, phy, phw, phh);
            float flashA = player.punchHit ? 0.9f : 0.55f;
            drawRect(phx, phy, phw, phh,
                     player.punchHit ? 1.0f : 1.0f,
                     player.punchHit ? 0.2f : 0.6f,
                     player.punchHit ? 0.2f : 0.0f,
                     flashA);
        }

        // Arc preview — fires AWAY from mouse (slingshot style)
        if (player.ammo > 0)
        {
            float t  = std::min(pullLen / MAX_POWER, 1.0f);
            float pr = 0.2f + t * 0.8f;
            float pg = 0.8f - t * 0.6f;
            float pb = 1.0f - t;

            // Line from muzzle to mouse cursor (shows the "pull" direction)
            drawLine(player.muzzleX(), player.muzzleY(),
                     mouseWorldX, mouseWorldY,
                     0.9f, 0.7f, 0.2f, 0.6f, 1.5f);

            // Arc in the OPPOSITE direction (away from mouse)
            drawArcPreview(player.muzzleX(), player.muzzleY(),
                           player.aimDirX * pullLen,   // already negated
                           player.aimDirY * pullLen,
                           pr, pg, pb);
        }

        // ==========================================================
        // RENDER — HUD (screen-space)
        // ==========================================================
        applyScreenSpace(sw, sh);

        // FPS counter
        {
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", currentFPS);
            drawRect((float)sw - 140.0f, 12.0f, 128.0f, 36.0f, 0.0f, 0.0f, 0.0f, 0.45f);
            font.draw(fpsText, (float)sw - 130.0f, 38.0f, 0.3f, 1.0f, 0.3f, 1.0f);
        }

        // ----------------------------------------------------------
        // Player HP bar — top-left, green label
        // ----------------------------------------------------------
        drawHpBar(font, "PLAYER", player.hp, player.maxHp,
                  20.0f, 16.0f, 220.0f, 18.0f,
                  0.4f, 1.0f, 0.4f);

        // ----------------------------------------------------------
        // Bot HP bar — top-right, red label
        // ----------------------------------------------------------
        drawHpBar(font, "BOT", bot.hp, bot.maxHp,
                  (float)sw - 340.0f, 16.0f, 220.0f, 18.0f,
                  1.0f, 0.4f, 0.4f);

        // Ammo display
        {
            bool outOfAmmo = (player.ammo <= 0);

            char countBuf[32];
            snprintf(countBuf, sizeof(countBuf), "AMMO  %d / %lld", player.ammo, (long long)MAX_AMMO);

            const float charW  = 16.0f;
            float       labelW = strlen(countBuf) * charW;
            float       labelX = (sw - labelW) * 0.5f;
            float       labelY = (float)sh - 24.0f;

            drawRect(labelX - 14.0f, labelY - 28.0f, labelW + 28.0f, 40.0f,
                     0.0f, 0.0f, 0.0f, 0.55f);

            font.draw(countBuf, labelX, labelY,
                      1.0f,
                      outOfAmmo ? 0.2f : 0.9f,
                      outOfAmmo ? 0.2f : 0.2f,
                      1.0f);

            if (outOfAmmo)
            {
                float pulse = fabsf(sinf(promptPulse * 3.0f));
                const char* punchHint = "[ RIGHT CLICK ] PUNCH";
                float hintW = strlen(punchHint) * charW;
                float hintX = (sw - hintW) * 0.5f;
                float hintY = labelY - 36.0f;
                drawRect(hintX - 10.0f, hintY - 26.0f, hintW + 20.0f, 36.0f,
                         0.0f, 0.0f, 0.0f, 0.5f);
                font.draw(punchHint, hintX, hintY, 1.0f, 0.3f + pulse * 0.4f, 0.1f, 1.0f);
            }
        }

        // Power bar
        if (player.ammo > 0)
        {
            float t = std::min(pullLen / MAX_POWER, 1.0f);

            const float barW = 200.0f;
            float barX = (sw - barW) * 0.5f;
            float barY =  sh - 52.0f;

            float pr = 0.2f + t * 0.8f;
            float pg = 0.8f - t * 0.6f;
            float pb = 1.0f - t;

            drawRect(barX - 4.0f, barY - 30.0f, barW + 8.0f, 52.0f, 0.0f, 0.0f, 0.0f, 0.45f);
            drawRect(barX, barY, barW, 16.0f, 0.15f, 0.15f, 0.15f, 0.9f);
            drawRect(barX, barY, barW * t, 16.0f, pr, pg, pb, 1.0f);

            char powerLabel[24];
            snprintf(powerLabel, sizeof(powerLabel), "POWER  %d", (int)pullLen);
            font.draw(powerLabel, barX, barY - 6.0f, pr, pg, pb, 1.0f);
        }

        // Punch cooldown bar
        if (player.punchCooldown > 0.0f)
        {
            const float barW = 120.0f;
            float barX = (sw - barW) * 0.5f;
            float barY =  sh - 80.0f;
            float fill = 1.0f - (player.punchCooldown / PUNCH_COOLDOWN);

            drawRect(barX - 4.0f, barY - 24.0f, barW + 8.0f, 42.0f, 0.0f, 0.0f, 0.0f, 0.45f);
            drawRect(barX, barY, barW, 12.0f, 0.15f, 0.15f, 0.15f, 0.9f);
            drawRect(barX, barY, barW * fill, 12.0f, 1.0f, 0.4f, 0.0f, 1.0f);
            font.draw("PUNCH", barX, barY - 4.0f, 1.0f, 0.5f, 0.1f, 1.0f);
        }

        PL_Present(&win);
    }

    // ==========================================================
    // CLEANUP
    // ==========================================================
    for (Bullet& b : bullets)
        if (b.sprite) DestroySprite(b.sprite);

    EventClear();

    font.destroy();
    worldFont.destroy();
    DestroyAnimator(player.anim);
    DestroySprite(player.sprite);
    DestroyAnimator(bot.anim);
    DestroySprite(bot.sprite);
    DestroySprite(background);
    DestroySound(sfxPunch);
    DestroySound(sfxJump);
    DestroySound(bgm);
    DestroySound(loadingMusic);
    PL_FreeImage(progressBarImg);
    PL_FreeImage(progressBorderImg);
    PL_FreeImage(loadingImage);
    FreeAtlas(atlas);
    PL_FreeImage(sheet);

    return 0;
}
