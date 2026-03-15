#pragma once
#include <cstdint>
#include <cmath>

namespace LightsOut {

// ─── Math types ──────────────────────────────────────────────────────────────
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }

    float length() const { return std::sqrt(x * x + y * y); }
    Vec2  normalized() const {
        float l = length();
        return l > 0.0f ? Vec2{x / l, y / l} : Vec2{0.0f, 0.0f};
    }
    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float distanceTo(const Vec2& o) const { return (*this - o).length(); }
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rect() = default;
    Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    bool contains(const Vec2& p) const {
        return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }
    bool intersects(const Rect& o) const {
        return x < o.x + o.w && x + w > o.x &&
               y < o.y + o.h && y + h > o.y;
    }
    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
};

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    static constexpr Color White()       { return {255, 255, 255}; }
    static constexpr Color Black()       { return {0, 0, 0}; }
    static constexpr Color Red()         { return {220, 50, 50}; }
    static constexpr Color Green()       { return {50, 200, 50}; }
    static constexpr Color Blue()        { return {50, 100, 220}; }
    static constexpr Color Yellow()      { return {255, 220, 50}; }
    static constexpr Color Orange()      { return {200, 120, 30}; }
    static constexpr Color NightSky()    { return {8, 12, 35}; }
    static constexpr Color SnowWhite()   { return {230, 240, 255}; }
    static constexpr Color WarmLight()   { return {255, 200, 100}; }
    static constexpr Color SquirrelBrown() { return {140, 85, 30}; }
};

// ─── Game states ─────────────────────────────────────────────────────────────
enum class GameState {
    MainMenu,
    Controls,        // rebind controls screen
    Playing,
    Paused,
    GameOver,
    Upgrade,
    Leaderboard,
    TownSquareBoss,  // alias kept for internal use — prefer GameState::Endgame
    Endgame,         // LEVEL_ENDGAME: big-tree finale (TownSquareBossScreen)
    YouWin           // victory screen
};

// ─── Lane types ──────────────────────────────────────────────────────────────
enum class LaneType {
    Rooftop = 0,
    Fence   = 1,
    Ground  = 2
};

// ─── Threat types ────────────────────────────────────────────────────────────
enum class ThreatType {
    Homeowner_Grumpy,
    Homeowner_Dad,
    Dog,
    Cat,
    Owl,
    NeighborhoodWatch,
    PorchLight    // not a chaser, just an alerter
};

// ─── Power-up types ──────────────────────────────────────────────────────────
enum class PowerUpType {
    AcornStash,    // speed boost
    WinterCoat,    // invincibility
    ShadowMode,    // invisible to threats
    SuperChomp,    // instant bite through tangled
    DecoyNut,      // distract homeowner/dog
    IcePatch,      // freeze threat
    DoubleTail,    // two squirrels briefly
    FrenzyMode     // bullet-time
};

// ─── Entity tags (bitmask for collision filtering) ───────────────────────────
enum EntityTag : uint32_t {
    TAG_NONE       = 0,
    TAG_PLAYER     = 1 << 0,
    TAG_LIGHT      = 1 << 1,
    TAG_THREAT     = 1 << 2,
    TAG_POWERUP    = 1 << 3,
    TAG_HOUSE      = 1 << 4,
    TAG_DECOY      = 1 << 5
};

// ─── Input actions ───────────────────────────────────────────────────────────
enum class Action {
    MoveLeft,
    MoveRight,
    Jump,       // launch squirrel upward (was MoveUp)
    Drop,       // fall through current platform (was MoveDown)
    Bite,
    UsePowerUp,
    Pause,
    MenuConfirm,
    MenuBack,
    MenuUp,
    MenuDown,
    Count
};

// ─── Upgrade types ───────────────────────────────────────────────────────────
enum class UpgradeType {
    BiteSpeed,     // reduce bite animation time
    MovementSpeed, // increase vertical lane-jump speed
    QuietSteps,    // reduce alert radius when on ground/fence
    JumpHeight,    // can jump two lanes at once
    Count
};

// ─── Squirrel upgrade state ──────────────────────────────────────────────────
struct SquirrelUpgrades {
    int biteSpeed    = 0;   // 0-3
    int moveSpeed    = 0;
    int quietSteps   = 0;
    int jumpHeight   = 0;

    static constexpr int MAX_LEVEL = 3;
};

// ─── House difficulty ─────────────────────────────────────────────────────────
enum class HouseDifficulty { Easy, Medium, Hard };

// ─── Level config ────────────────────────────────────────────────────────────
struct LevelConfig {
    const char* name;
    float scrollSpeed;
    float threatDensity;        // 0.0 – 1.0
    float powerUpFrequency;     // 0.0 – 1.0
    bool  homeownersCoordinate;
    bool  hasNeighborhoodWatch;
    int   lightStringsPerHouse; // strands per tier on hard houses
    float tangledLightProbability;
    float darkHouseProbability; // chance a house is completely undecorated
    float easyHouseFraction;    // of lit houses: 1-tier (roof OR window OR porch)
    float mediumHouseFraction;  // of lit houses: 2 adjacent tiers; hard = remainder
};

}  // namespace LightsOut
