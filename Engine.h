#pragma once
// Engine.h
// Wraps the RubberBandBattle-Shell game loop into a reusable Engine class.
// Build 0.0.19 -> refactor

#include "PlaylabsGL.h"
#include "Src/UI.h"
#include <vector>
#include <string>

#include "Constants.h"
#include "Bullet.h"
#include "Player.h"

// A single registered player skin (spritesheet image + atlas JSON pair).
struct SkinDef
{
    std::string name;
    std::string imagePath;
    std::string atlasPath;
};

class Engine
{
public:
    Engine()  = default;
    ~Engine() = default;

    // ---- Lifecycle ----
    // imagePath/atlasPath select the player's starting skin; loaded externally rather
    // than hardcoded, so callers can point at any spritesheet/atlas pair.
    bool Init(const char* imagePath = "Resources/Skins/spritemap.png",
              const char* atlasPath = "Resources/Skins/spritemap.json");
    void Run();      // blocks: runs the main loop until the window closes
    void Shutdown(); // frees sounds/sprites/atlases/debug panel

    // ---- Skin loading / switching ----
    // Loads a spritesheet + atlas pair and applies it to the player (and background)
    // sprites immediately, freeing whatever was loaded before. Returns false if either
    // asset fails to load, in which case the previous skin remains active.
    bool LoadPlayerAtlas(const char* imagePath, const char* atlasPath);

    // Adds a skin to the switchable roster (does not load it). Call SwitchSkin*/Next/Prev
    // afterwards to actually apply it.
    void RegisterSkin(const char* name, const char* imagePath, const char* atlasPath);

    bool SwitchSkin(int index);                 // apply a registered skin by index
    bool SwitchSkinByName(const char* name);     // apply a registered skin by name
    void NextSkin();                             // cycle forward through registered skins
    void PrevSkin();                             // cycle backward through registered skins

    int         GetSkinCount()       const { return (int)skins.size(); }
    int         GetCurrentSkinIndex() const { return currentSkinIndex; }
    const char* GetCurrentSkinName()  const;

    // ---- Gameplay actions (safe to call from anywhere, e.g. debug panel / scripting) ----
    void Jump();
    void Punch();
    void Shoot();               // fires toward the current aim direction at current pull power
    void TakeDamage(float dmg);
    void Kill();                // force death (skips hp check)
    void Respawn();             // immediately respawn, ignoring the countdown
    void SetSpawnPoint(float x, float y);

    // ---- Accessors ----
    Player&       GetPlayer()        { return player; }
    const Player& GetPlayer()  const { return player; }
    Camera&       GetCamera()        { return main_cam; }
    bool          IsDead()     const { return isDead; }
    float         GetStamina() const { return stamina; }
    float         GetMaxStamina() const { return MAX_STAMINA; }
    bool          IsStaminaExhausted() const { return staminaExhausted; }
    float         GetFPS()     const { return currentFPS; }
    float         GetRespawnTimeRemaining() const { return respawnTimer; }
    Window&       GetWindow()        { return window; }

private:
    // ---- Per-frame steps ----
    void beginFrame();
    void pollInputAndDebug();
    void updateDeathState(float dtSeconds);
    void updateGameplay(float dtSeconds);

    // updateGameplay sub-steps
    void updateAiming(float dtSeconds, float mouseWorldX, float mouseWorldY, float& pullLen);
    void handleShootInput(float dtSeconds, float pullLen);
    void handlePunchInput();
    void tickPunch(float dtSeconds);
    void readMovementInput(bool& moving, float& moveX, float& moveY);
    void updateStamina(float dtSeconds, bool shiftHeld, bool moving, bool& canRun);
    void handleJumpInput();
    void applyPhysics(float dtSeconds, float moveX, float moveY, bool moving, bool canRun);
    void updateCameraFollow(float dtSeconds);
    void updateBullets(float dtSeconds);
    void applyAnimationState(bool moving, bool canRun);
    void handleSkinSwitchInput(); // 'T' / 'Y' cycle next/prev registered skin

    // ---- Rendering ----
    void renderWorld(float dtSeconds);
    void renderHUD();
    void endFrame();

    // ---- Core systems ----
    Window window;
    Camera main_cam;
    Timer  timer;

    int   sw = 1280, sh = 720;

    float fpsTimer    = 0.0f;
    float currentFPS  = 0.0f;
    int   fpsFrames   = 0;
    float promptPulse = 0.0f;
    float dt          = 0.0f;

    Sound* bgm      = nullptr;
    Sound* sfxJump  = nullptr;
    Sound* sfxPunch = nullptr;

    Image* playersheet = nullptr;
    Atlas* playeratlas = nullptr;

    std::vector<SkinDef> skins;
    int currentSkinIndex = -1;

    Player  player;
    Sprite* background = nullptr;

    std::vector<Bullet> bullets;

    bool prevMouseHeld  = false;
    bool prevRMouseHeld = false;

    Player::State lastAnimState = Player::State::IDLE;

    // Death / respawn
    bool  isDead         = false;
    float respawnTimer   = 0.0f;
    bool  deathAnimDone  = false;
    float deathAnimTimer = 0.0f;
    const float RESPAWN_DELAY  = 3.0f;
    const float DEATH_ANIM_DUR = 1.0f;

    // Spawn position - mutable so debug panel sliders / SetSpawnPoint can change it
    float SPAWN_X = 200.0f;
    float SPAWN_Y = 436.0f;

    // Stamina
    const float MAX_STAMINA       = 100.0f;
    const float STAMINA_RUN_DRAIN =  20.0f; // per second while running
    const float STAMINA_JMP_DRAIN =  15.0f; // flat cost per jump
    const float STAMINA_RECOVER   =  12.0f; // per second when not running
    float stamina          = MAX_STAMINA;
    bool  staminaExhausted = false;
};
