#include "PlaylabsGL.h"
#include <GL/gl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>
#include <algorithm>
#include <windows.h>


#define STB_TRUETYPE_IMPLEMENTATION
#include "include/stb_truetype.h"

//Build 0.0.9
// =============================================================
// CONSTANTS
// =============================================================
static const float PI             = 3.14159265f;
static const float BULLET_GRAVITY = 600.0f;
static const float MIN_POWER      = 200.0f;
static const float MAX_POWER      = 900.0f;
static const float CHARGE_RATE    = MAX_POWER / 1.2f;
static const float SHOOT_COOLDOWN = 0.15f;
static const float BULLET_LIFETIME= 4.0f;
static const float ARROW_SIZE     = 12.0f;
static const int   ARC_SEGMENTS   = 40;
static const float ARC_STEP_T     = 0.08f;
static const int   MAX_AMMO       = 100;

// Punch constants
static const float PUNCH_RANGE    = 80.0f;   // how far the hitbox reaches
static const float PUNCH_DURATION = 0.25f;   // how long the punch hitbox is active
static const float PUNCH_COOLDOWN = 0.45f;   // time before next punch

// Loading bar constants
static const float LOAD_BAR_W     = 480.0f;  // width of the progress bar image
static const float LOAD_BAR_H     = 32.0f;   // height of the progress bar image
static const float LOAD_FILL_RATE = 0.55f;   // units/sec — how fast the bar auto-fills
                                              // Set to 0 to drive it manually from asset loading

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

// Draws the progress bar using the two sprite images.
// fillImg  : fill/color image (progress_bar.png) — clipped to [0, fill]
// borderImg: border/frame image (progress_border.png) — always drawn at full width on top
// fill     : 0.0 – 1.0
static void drawProgressBar(Image* fillImg, Image* borderImg,
                             float x, float y, float w, float h,
                             float fill, float alpha = 1.0f)
{
    // --- clipped fill drawn first (sits behind the border) ---
    if (fillImg && fill > 0.0f)
    {
        float fw = w * fill;
        float u1 = fill;   // UV right edge matches the fill ratio

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

    // --- border/frame drawn on top at full width always ---
    drawImageStretched(borderImg, x, y, w, h, alpha);
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

    bool  charging      = false;
    float chargeTimer   = 0.0f;
    float shootCooldown = 0.0f;
    int   ammo          = MAX_AMMO;

    float aimDirX = 1.0f;
    float aimDirY = 0.0f;

    // --- Punch state ---
    bool  punching       = false;
    float punchTimer     = 0.0f;
    float punchCooldown  = 0.0f;
    bool  punchHit       = false;

    Sprite*           sprite = nullptr;
    TimelineAnimator* anim   = nullptr;

    enum class State { IDLE, RUN, JUMP, PUNCH } state = State::IDLE;
    float facingX = 1.0f;

    float muzzleX() const { return x + w * 0.5f; }
    float muzzleY() const { return y + h * 0.5f; }

    void getPunchHitbox(float& hx, float& hy, float& hw, float& hh) const
    {
        hw = PUNCH_RANGE;
        hh = 50.0f;
        if (facingX < 0.0f)  // facing right
            hx = x + w;
        else                  // facing left
            hx = x - hw;
        hy = y + h * 0.2f;
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
        endX = nx;  endY = ny;
        px = nx; py = ny;
        pvx = pvx; pvy = nvy;
    }

    float dx = endX - prevX;
    float dy = endY - prevY;
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.0001f) { dx /= len; dy /= len; }

    float tx  = endX + dx * ARROW_SIZE;
    float ty  = endY + dy * ARROW_SIZE;
    float px1 = endX - dy * ARROW_SIZE * 0.5f;
    float py1 = endY + dx * ARROW_SIZE * 0.5f;
    float px2 = endX + dy * ARROW_SIZE * 0.5f;
    float py2 = endY - dx * ARROW_SIZE * 0.5f;

    drawTriangle(tx, ty, px1, py1, px2, py2, r, g, b, 1.0f);
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

    // FPS counter
    float fpsTimer   = 0.0f;
    int   fpsFrames  = 0;
    float currentFPS = 0.0f;

    // ---------------------------------------------------------
    // Font
    // ---------------------------------------------------------
    FontRenderer font;
    font.load("Resources/Font/Confale.ttf", 28.0f);

    // ---------------------------------------------------------
    // Loading screen assets
    // ---------------------------------------------------------
    enum class GameState { LOADING, PLAYING };
    GameState gameState = GameState::LOADING;

    Image* loadingImage  = Playlabs_LoadImage("loading.png");
    Image* progressBorderImg = Playlabs_LoadImage("progress_border.png");
    Image* progressBarImg= Playlabs_LoadImage("progress_bar.png");

    Sound* loadingMusic = Playlabs_CreateSound();
    loadingMusic->load("loading.wav");
    loadingMusic->play(true);

    float loadingFade       = 10.0f;
    bool  loadingFadingOut  = false;
    const float FADE_SPEED  = 2.5f;

    float promptPulse  = 1.0f;
    bool  prevAnyKeyDown = false;

    // Loading-bar state
    float loadProgress  = 0.0f;   // 0.0 → 1.0
    bool  loadComplete  = false;   // true once bar reaches 1.0

    // ---------------------------------------------------------
    // Game assets
    // ---------------------------------------------------------
    Image* sheet = Playlabs_LoadImage("Resources/Skins/spritemap.png");
    Atlas* atlas = Playlabs_LoadAtlas("Resources/Skins/spritemap.json");

    Sound* bgm = Playlabs_CreateSound();
    bgm->load("loading.wav");

    Sound* sfxJump = Playlabs_CreateSound();
    sfxJump->load("jump.wav");

    Sound* sfxPunch = Playlabs_CreateSound();
    sfxPunch->load("punch.wav");

    // ---------------------------------------------------------
    // Player
    // ---------------------------------------------------------
    Player player;

    player.sprite              = Playlabs_CreateSprite();
    player.sprite->image       = sheet;
    player.sprite->atlas       = atlas;
    player.sprite->frameName   = "0010";
    player.sprite->position      = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {1.0f, 1.0f};
    player.sprite->targetScale    = {1.0f, 1.0f};

    player.anim = Playlabs_CreateAnimator();
    if (player.anim->load("Resources/Skins/Animation.json"))
        Playlabs_Anim(player.anim, PLAYER, IDLE);

    // ---------------------------------------------------------
    // Background
    // ---------------------------------------------------------
    Sprite* background         = Playlabs_CreateSprite();
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

        // FPS update
        fpsTimer += dt;
        fpsFrames++;
        if (fpsTimer >= 1.0f)
        {
            currentFPS = fpsFrames / fpsTimer;
            fpsFrames  = 0;
            fpsTimer   = 0.0f;
        }

        Playlabs_PollInput(&win);

        int sw = win.getWidth();
        int sh = win.getHeight();

        // ======================================================
        // LOADING SCREEN
        // ======================================================
        if (gameState == GameState::LOADING)
        {
            promptPulse += dt * 1.0f;

            // --- Advance the loading bar ---
            // Replace this block with actual asset-load progress if desired.
            if (!loadComplete)
            {
                loadProgress += LOAD_FILL_RATE * dt;
                if (loadProgress >= 1.0f)
                {
                    loadProgress = 1.0f;
                    loadComplete = true;
                }
            }

            // Only accept input once the bar is full
            bool anyKeyDown = loadComplete &&
                              (Playlabs_KeyDown(VK_LBUTTON) ||
                               Playlabs_KeyDown(VK_RETURN)  ||
                               Playlabs_KeyDown(VK_SPACE)   ||
                               Playlabs_KeyDown(VK_ESCAPE));

            bool justPressed = anyKeyDown && !prevAnyKeyDown;
            prevAnyKeyDown   = anyKeyDown;

            if (justPressed && !loadingFadingOut)
                loadingFadingOut = true;

            if (loadingFadingOut)
            {
                loadingFade -= dt * FADE_SPEED;
                if (loadingFade <= 0.0f)
                {
                    loadingFade  = 0.0f;
                    gameState    = GameState::PLAYING;
                    loadingMusic->stop();
                    bgm->play(true);
                }
            }

            Playlabs_Clear(0.0f, 0.0f, 0.0f, 1.0f);
            applyScreenSpace(sw, sh);

            // Loading screen background
            drawImageStretched(loadingImage, 0.0f, 0.0f, (float)sw, (float)sh, loadingFade);

            // --------------------------------------------------
            // Progress bar:
            //   fill   = progress_bar.png    (clipped to loadProgress)
            //   border = progress_border.png (full width, drawn on top)
            // --------------------------------------------------
            {
                float barW = LOAD_BAR_W;
                float barH = LOAD_BAR_H;
                float barX = (sw - barW) * 0.5f;
                float barY =  sh * 0.82f;

                drawProgressBar(progressBarImg, progressBorderImg,
                                barX, barY, barW, barH,
                                loadProgress, loadingFade);

                // Percentage label centred above the bar
                char pctBuf[16];
                snprintf(pctBuf, sizeof(pctBuf), "%d%%",
                         (int)(loadProgress * 100.0f));
                float labelX = barX + barW * 0.5f - strlen(pctBuf) * 8.0f;
                float labelY = barY - 6.0f;
                font.draw(pctBuf, labelX, labelY,
                          1.0f, 1.0f, 1.0f, 0.85f * loadingFade);
            }

            // "PRESS ANY KEY" hint — only shown once loading is complete
            if (!loadingFadingOut && loadComplete)
            {
                float pulse = (sinf(promptPulse) * 1.5f + 1.5f);
                float hintA = 0.5f + pulse * 0.5f;

                const char* hint    = "PRESS ANY KEY TO START";
                float        hintX  = sw * 0.5f - strlen(hint) * 8.0f;
                float        hintY  = sh * 0.5f - strlen(hint) * 4.0f;

                drawRect(hintX - 16.0f, hintY - 26.0f,
                         strlen(hint) * 16.0f + 32.0f, 38.0f,
                         0.0f, 0.0f, 0.0f, 0.55f * loadingFade);

                font.draw(hint, hintX, hintY,
                          1.0f, 1.0f, 1.0f, hintA * loadingFade);
            }

            Playlabs_Present(&win);
            continue;
        }

        // ======================================================
        // GAME LOGIC
        // ======================================================

        int mousePixelX = 0, mousePixelY = 0;
        Playlabs_MousePos(&mousePixelX, &mousePixelY);

        float mouseWorldX, mouseWorldY;
        screenToWorld((float)mousePixelX, (float)mousePixelY,
                      cam, sw, sh,
                      mouseWorldX, mouseWorldY);

        float aimDX = mouseWorldX - player.muzzleX();
        float aimDY = mouseWorldY - player.muzzleY();
        float aimLen = sqrtf(aimDX * aimDX + aimDY * aimDY);
        if (aimLen > 0.001f)
        {
            player.aimDirX = aimDX / aimLen;
            player.aimDirY = aimDY / aimLen;
        }

        player.facingX = (player.aimDirX > 0.0f) ? -1.0f : 1.0f;

        player.shootCooldown  -= dt;
        player.punchCooldown  -= dt;

        // ------------------------------------------------------
        // LEFT MOUSE — shoot (only when ammo > 0)
        // ------------------------------------------------------
        bool mouseHeld    = Playlabs_KeyDown(VK_LBUTTON) != 0;
        bool justReleased = prevMouseHeld && !mouseHeld;
        prevMouseHeld     = mouseHeld;

        if (player.ammo > 0)
        {
            if (mouseHeld && player.shootCooldown <= 0.0f)
            {
                player.charging    = true;
                player.chargeTimer += dt * (CHARGE_RATE / MAX_POWER);
                if (player.chargeTimer > 1.0f) player.chargeTimer = 1.0f;
            }

            if (justReleased && player.charging)
            {
                float power = MIN_POWER + (MAX_POWER - MIN_POWER) * player.chargeTimer;

                Bullet b;
                b.x    = player.muzzleX() - 24.0f;
                b.y    = player.muzzleY() - 24.0f;
                b.velX = player.aimDirX * power;
                b.velY = player.aimDirY * power;

                b.sprite              = Playlabs_CreateSprite();
                b.sprite->image       = sheet;
                b.sprite->atlas       = atlas;
                b.sprite->frameName   = "0000";
                b.sprite->scale       = {player.facingX, 1.0f};
                b.sprite->targetScale = {player.facingX, 1.0f};
                b.sprite->position      = {b.x, b.y};
                b.sprite->targetPosition = {b.x, b.y};

                bullets.push_back(b);
                player.ammo--;

                player.charging    = false;
                player.chargeTimer = 0.0f;
                player.shootCooldown = SHOOT_COOLDOWN;
            }
        }
        else
        {
            player.charging    = false;
            player.chargeTimer = 0.0f;
        }

        // ------------------------------------------------------
        // RIGHT MOUSE — punch
        // ------------------------------------------------------
        bool rMouseHeld     = Playlabs_KeyDown(VK_RBUTTON) != 0;
        bool rJustPressed   = rMouseHeld && !prevRMouseHeld;
        prevRMouseHeld      = rMouseHeld;

        if (rJustPressed && !player.punching && player.punchCooldown <= 0.0f && !player.jumping)
        {
            player.punching   = true;
            player.punchTimer = 0.0f;
            player.punchHit   = false;

            Playlabs_Anim(player.anim, PLAYER, PUNCH);
            player.state = Player::State::PUNCH;

            sfxPunch->play(false);
        }

        if (player.punching)
        {
            player.punchTimer += dt;

            float wX = 100.0f, wY = 100.0f, wW = 100.0f, wH = 100.0f;

            if (!player.punchHit)
            {
                float phx, phy, phw, phh;
                player.getPunchHitbox(phx, phy, phw, phh);

                if (Playlabs_AABBIntersects(phx, phy, phw, phh, wX, wY, wW, wH))
                {
                    player.punchHit = true;
                }
            }

            if (player.punchTimer >= PUNCH_DURATION)
            {
                player.punching      = false;
                player.punchCooldown = PUNCH_COOLDOWN;
                player.state = Player::State::IDLE;
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
            if (Playlabs_KeyDown(VK_RIGHT) || Playlabs_KeyDown('D')) { moveX =  1.0f; moving = true; }
            if (Playlabs_KeyDown(VK_LEFT)  || Playlabs_KeyDown('A')) { moveX = -1.0f; moving = true; }

            if (!player.jumping)
            {
                if (Playlabs_KeyDown(VK_UP)   || Playlabs_KeyDown('W')) { moveY = -1.0f; moving = true; }
                if (Playlabs_KeyDown(VK_DOWN) || Playlabs_KeyDown('S')) { moveY =  1.0f; moving = true; }
            }
        }

        if (Playlabs_KeyPressed(VK_SPACE) && !player.jumping && !player.punching)
        {
            player.baseY     = player.y;
            player.velocityY = player.jumpForce;
            player.jumping   = true;
            player.state     = Player::State::JUMP;
            Playlabs_Anim(player.anim, PLAYER, JUMP);
            sfxJump->play(false);
        }

        player.x += moveX * player.speed * dt;

        if (!player.jumping)
        {
            player.y += moveY * player.speed * dt;
            player.y  = (player.y < player.minY)            ? player.minY
                      : (player.y > player.maxY - player.h) ? player.maxY - player.h
                      : player.y;
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
                    Playlabs_Anim(player.anim, PLAYER, moving ? RUN : IDLE);
                }
            }
        }

        player.sprite->position      = {player.x, player.y};
        player.sprite->targetPosition = {player.x, player.y};
        player.sprite->scale          = {player.facingX, 1.0f};
        player.sprite->targetScale    = {player.facingX, 1.0f};

        if (!player.jumping && !player.punching)
        {
            if (moving && player.state != Player::State::RUN)
            {
                player.state = Player::State::RUN;
                Playlabs_Anim(player.anim, PLAYER, RUN);
            }
            else if (!moving && player.state != Player::State::IDLE)
            {
                player.state = Player::State::IDLE;
                Playlabs_Anim(player.anim, PLAYER, IDLE);
            }
        }

        Playlabs_SetAnimatorParent(player.anim,
            player.x, player.y, 0.0f, player.facingX, 1.0f);

        float targetCamX = player.x - sw * 0.5f + 340.0f;
        float targetCamY = player.y - sh * 0.5f + 280.0f;
        cam.position.x  += (targetCamX - cam.position.x) * 5.0f * dt;
        cam.position.y  += (targetCamY - cam.position.y) * 5.0f * dt;

        float wX = 100.0f, wY = 100.0f, wW = 100.0f, wH = 100.0f;

        for (Bullet& b : bullets)
        {
            if (b.dead) continue;

            b.velY += BULLET_GRAVITY * dt;
            b.x    += b.velX * dt;
            b.y    += b.velY * dt;

            b.sprite->position      = {b.x, b.y};
            b.sprite->targetPosition = {b.x, b.y};
            b.sprite->update(dt);

            b.life -= dt;
            if (b.life <= 0.0f) { b.dead = true; continue; }

            if (Playlabs_AABBIntersects(b.hitX(), b.hitY(), b.hitW, b.hitH,
                                         wX, wY, wW, wH))
                b.dead = true;
        }

        for (Bullet& b : bullets)
            if (b.dead && b.sprite) { Playlabs_DestroySprite(b.sprite); b.sprite = nullptr; }
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                           [](const Bullet& b){ return b.dead; }),
            bullets.end());

        float playerHitboxSizeX = 100.0f;
        float playerHitboxSizeY = 200.0f;
        float hitW = player.jumping ? playerHitboxSizeX * 0.6f : playerHitboxSizeX;
        float hitH = player.jumping ? playerHitboxSizeY * 0.5f : playerHitboxSizeY;
        float hitX = player.x + (player.w - 200.0f) * 0.5f;
        float hitY = player.y + player.h;

        if (Playlabs_AABBIntersects(hitX, hitY, hitW, hitH, wX, wY, wW, wH))
        {
            player.x -= moveX * player.speed * dt;
            player.sprite->position      = {player.x, player.y};
            player.sprite->targetPosition = {player.x, player.y};
        }

        // ==========================================================
        // RENDER — world
        // ==========================================================
        Playlabs_Clear(0.12f, 0.12f, 0.18f, 1.0f);
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
        Playlabs_TickAnimator(player.anim, dt, sheet, atlas, &cam);

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

        if (player.charging && player.ammo > 0)
        {
            float power = MIN_POWER + (MAX_POWER - MIN_POWER) * player.chargeTimer;
            float t  = player.chargeTimer;
            float pr = 0.2f + t * 0.8f;
            float pg = 0.8f - t * 0.6f;
            float pb = 1.0f - t * 1.0f;
            drawArcPreview(player.muzzleX(), player.muzzleY(),
                           player.aimDirX * power,
                           player.aimDirY * power,
                           pr, pg, pb);
        }

        // ==========================================================
        // RENDER — HUD (screen-space)
        // ==========================================================
        applyScreenSpace(sw, sh);


        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", currentFPS);
        drawRect((float)sw - 140.0f, 12.0f, 128.0f, 36.0f,
                 0.0f, 0.0f, 0.0f, 0.45f);
        font.draw(fpsText, (float)sw - 130.0f, 38.0f,
                  0.3f, 1.0f, 0.3f, 1.0f);

        // --- Ammo display: centered, number only ---
        {
            bool outOfAmmo = (player.ammo <= 0);

            // "AMMO  XX / 100" centered at bottom-center
            char countBuf[32];
            snprintf(countBuf, sizeof(countBuf), "AMMO  %d / %d", player.ammo, MAX_AMMO);

            const float charW  = 16.0f;   // approximate glyph advance at size 28
            float labelW = strlen(countBuf) * charW;
            float labelX = (sw - labelW) * 0.5f;
            float labelY = (float)sh - 24.0f;   // near bottom center

            // Dark pill background
            drawRect(labelX - 14.0f, labelY - 28.0f,
                     labelW + 28.0f, 40.0f,
                     0.0f, 0.0f, 0.0f, 0.55f);

            font.draw(countBuf, labelX, labelY,
                      outOfAmmo ? 1.0f : 1.0f,
                      outOfAmmo ? 0.2f : 0.9f,
                      outOfAmmo ? 0.2f : 0.2f,
                      1.0f);

            // "PUNCH" hint when out of ammo
            if (outOfAmmo)
            {
                float pulse = fabsf(sinf(promptPulse * 3.0f));
                const char* punchHint = "[ RIGHT CLICK ] PUNCH";
                float hintW = strlen(punchHint) * charW;
                float hintX = (sw - hintW) * 0.5f;
                float hintY = labelY - 36.0f;
                drawRect(hintX - 10.0f, hintY - 26.0f,
                         hintW + 20.0f, 36.0f,
                         0.0f, 0.0f, 0.0f, 0.5f);
                font.draw(punchHint, hintX, hintY,
                          1.0f, 0.3f + pulse * 0.4f, 0.1f, 1.0f);
            }
        }

        if (player.charging && player.ammo > 0)
        {
            const float barW = 200.0f;
            float barX = (sw - barW) * 0.5f;
            float barY =  sh - 52.0f;

            float t  = player.chargeTimer;
            float pr = 0.2f + t * 0.8f;
            float pg = 0.8f - t * 0.6f;
            float pb = 1.0f - t * 1.0f;

            drawRect(barX - 4.0f, barY - 30.0f, barW + 8.0f, 52.0f,
                     0.0f, 0.0f, 0.0f, 0.45f);
            drawRect(barX, barY, barW, 16.0f, 0.15f, 0.15f, 0.15f, 0.9f);
            drawRect(barX, barY, barW * player.chargeTimer, 16.0f, pr, pg, pb, 1.0f);

            char chargeLabel[20];
            snprintf(chargeLabel, sizeof(chargeLabel), "POWER  %d%%",
                     (int)(player.chargeTimer * 100.0f));
            font.draw(chargeLabel, barX, barY - 6.0f, pr, pg, pb, 1.0f);
            drawRect(barX, barY, barW, 16.0f, 0.15f, 0.15f, 0.15f, 0.9f);
        }

        if (player.punchCooldown > 0.0f)
        {
            const float barW = 120.0f;
            float barX = (sw - barW) * 0.5f;
            float barY =  sh - 80.0f;
            float fill = 1.0f - (player.punchCooldown / PUNCH_COOLDOWN);

            drawRect(barX - 4.0f, barY - 24.0f, barW + 8.0f, 42.0f,
                     0.0f, 0.0f, 0.0f, 0.45f);
            drawRect(barX, barY, barW, 12.0f, 0.15f, 0.15f, 0.15f, 0.9f);
            drawRect(barX, barY, barW * fill, 12.0f, 1.0f, 0.4f, 0.0f, 1.0f);
            font.draw("PUNCH", barX, barY - 4.0f, 1.0f, 0.5f, 0.1f, 1.0f);
        }

        Playlabs_Present(&win);
    }

    // ==========================================================
    // CLEANUP
    // ==========================================================
    for (Bullet& b : bullets)
        if (b.sprite) Playlabs_DestroySprite(b.sprite);

    font.destroy();
    Playlabs_DestroyAnimator(player.anim);
    Playlabs_DestroySprite(player.sprite);
    Playlabs_DestroySprite(background);
    Playlabs_DestroySound(sfxPunch);
    Playlabs_DestroySound(sfxJump);
    Playlabs_DestroySound(bgm);
    Playlabs_DestroySound(loadingMusic);
    Playlabs_FreeImage(progressBarImg);
    Playlabs_FreeImage(progressBorderImg);
    Playlabs_FreeImage(loadingImage);
    Playlabs_FreeAtlas(atlas);
    Playlabs_FreeImage(sheet);

    return 0;
}
