#pragma once
#include "Entity.h"
#include "Lane.h"
#include "core/Types.h"
#include <functional>
#include <vector>
#include <memory>

namespace LightsOut {

class LightString;

enum class PlayerState {
    Idle,
    Running,    // on a platform or ground
    Airborne,   // in the air (jumping or falling)
    Biting,     // chomping a light string
    Stunned,    // briefly after failed bite
    Dead
};

// Keep AnimFrame enum for future use (not all used in current sprites)
enum class AnimFrame {
    Idle0, Idle1,
    Run0, Run1, Run2, Run3,
    Jump,
    Bite,
    Shadow,
    Count
};

class Player : public Entity {
public:
    Player();

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    // Input-driven actions
    void moveHorizontal(float dir, float dt);  // dir: -1=left, +1=right
    void jump();                                // launch upward if grounded
    void drop();                                // fall through current platform
    void tryBite(const std::vector<std::shared_ptr<LightString>>& nearbyStrings, float cameraX);

    // Called by GameWorld platform resolver each frame
    void landOnPlatform(float platformY, LaneType tier);
    void setGrounded(bool grounded) { m_isGrounded = grounded; }

    // Reset player state for a new life
    void respawn();

    // Power-up application
    void applySpeedBoost(float multiplier, float duration);
    void applyInvincibility(float duration);
    void applyShadowMode(float duration);
    void applySuperChomp(float duration);
    void applyFrenzyMode(float duration);
    void applyDoubleTail(float duration);

    bool isInvincible()    const { return m_invincibleTimer > 0.0f; }
    bool isShadowMode()    const { return m_shadowTimer > 0.0f; }
    bool isSuperChomp()    const { return m_superChompTimer > 0.0f; }
    bool isFrenzy()        const { return m_frenzyTimer > 0.0f; }
    bool isDoubleTail()    const { return m_doubleTailTimer > 0.0f; }
    bool isGrounded()      const { return m_isGrounded; }

    // Drop state — GameWorld reads this to know which tier to ignore
    bool      isDropping()       const { return m_dropping; }
    LaneType  dropIgnoreTier()   const { return m_dropIgnoreTier; }
    void      tickDropTimer(float dt);  // called by GameWorld each frame

    LaneType    currentLane() const { return m_currentLane; }
    void        setCurrentLane(LaneType lt) { m_currentLane = lt; }
    PlayerState state()       const { return m_state; }
    float       screenX()     const { return m_screenX; }
    void        setScreenX(float x) { m_screenX = x; }
    float       velocityY()   const { return m_velocityY; }
    float       prevFeetY()   const { return m_prevFeetY; }

    // Called each frame by GameWorld: drives idle/run animation state
    void setMoving(bool moving);

    float frenzySlowFactor() const;

    void setUpgrades(const SquirrelUpgrades& u) { m_upgrades = u; }
    const SquirrelUpgrades& upgrades() const { return m_upgrades; }

    // Callbacks
    std::function<void()>           onDeath;
    std::function<void(int, bool)>  onBiteSuccess;

private:
    void updateTimers(float dt);
    void updateAnimation(float dt);
    void drawSquirrel(SDL_Renderer* renderer, float screenX, float screenY, bool shadow) const;
    void renderPowerUpGlow(SDL_Renderer* renderer, float screenX) const;

    LaneType    m_currentLane   = LaneType::Ground;
    PlayerState m_state         = PlayerState::Idle;
    float       m_screenX       = 50.0f;
    bool        m_facingLeft    = false;  // true → sprite rendered flipped horizontally

    // Physics
    float       m_velocityY     = 0.0f;
    bool        m_isGrounded    = false;
    float       m_prevFeetY     = 0.0f;   // feet Y from last frame (for sweep)

    // Drop-through state
    bool        m_dropping         = false;
    float       m_dropTimer        = 0.0f;
    LaneType    m_dropIgnoreTier   = LaneType::Ground;

    // Power-up timers
    float m_speedBoostTimer    = 0.0f;
    float m_speedBoostMult     = 1.0f;
    float m_invincibleTimer    = 0.0f;
    float m_shadowTimer        = 0.0f;
    float m_superChompTimer    = 0.0f;
    float m_frenzyTimer        = 0.0f;
    float m_doubleTailTimer    = 0.0f;

    // Animation
    float     m_animTimer    = 0.0f;
    int       m_animFrame    = 0;
    float     m_animSpeed    = 8.0f;

    // Bite animation
    float     m_biteTimer      = 0.0f;
    float     m_biteFrameTimer = 0.0f;
    int       m_biteFrame      = 0;

    SquirrelUpgrades m_upgrades;

    float m_slipTimer = 0.0f;
};

}  // namespace LightsOut
