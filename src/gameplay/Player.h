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
    Running,    // on ground/fence
    Jumping,    // transitioning between lanes
    Biting,     // chomping a light string
    Stunned,    // briefly after failed bite
    Dead
};

enum class AnimFrame {
    Idle0, Idle1,
    Run0, Run1, Run2, Run3,
    Jump,
    Bite,
    Shadow,    // semi-transparent overlay when Shadow Mode active
    Count
};

class Player : public Entity {
public:
    Player();

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    // Input-driven actions
    void moveUp();
    void moveDown();
    void tryBite(const std::vector<std::shared_ptr<LightString>>& nearbyStrings, float cameraX);

    // Reset player state for a new life (keeps camera position)
    void respawn();

    // Power-up application
    void applySpeedBoost(float multiplier, float duration);
    void applyInvincibility(float duration);
    void applyShadowMode(float duration);
    void applySuperChomp(float duration);
    void applyFrenzyMode(float duration);  // slows world, not player
    void applyDoubleTail(float duration);  // spawns a decoy player

    // Returns true if any invincibility is active
    bool isInvincible()    const { return m_invincibleTimer > 0.0f; }
    bool isShadowMode()    const { return m_shadowTimer > 0.0f; }
    bool isSuperChomp()    const { return m_superChompTimer > 0.0f; }
    bool isFrenzy()        const { return m_frenzyTimer > 0.0f; }
    bool isDoubleTail()    const { return m_doubleTailTimer > 0.0f; }

    LaneType    currentLane() const { return m_currentLane; }
    PlayerState state()       const { return m_state; }

    float frenzySlowFactor() const;  // 1.0 normally, < 1.0 in frenzy

    // Upgrades
    void setUpgrades(const SquirrelUpgrades& u) { m_upgrades = u; }
    const SquirrelUpgrades& upgrades() const { return m_upgrades; }

    // Callbacks
    std::function<void()>           onDeath;
    std::function<void(int, bool)>  onBiteSuccess;   // (bulbCount, wasChain)

private:
    void updateLaneTransition(float dt);
    void updateTimers(float dt);
    void updateAnimation(float dt);
    void renderSprite(SDL_Renderer* renderer, float screenX) const;
    void renderPowerUpGlow(SDL_Renderer* renderer, float screenX) const;
    void drawSquirrel(SDL_Renderer* renderer, float screenX, float screenY, bool shadow) const;

    LaneType    m_currentLane   = LaneType::Ground;
    LaneType    m_targetLane    = LaneType::Ground;
    PlayerState m_state         = PlayerState::Idle;
    float       m_jumpProgress  = 0.0f;  // 0→1 during lane transition
    float       m_jumpStartY    = 0.0f;
    float       m_jumpTargetY   = 0.0f;

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
    float     m_animSpeed    = 8.0f;  // frames per second

    SquirrelUpgrades m_upgrades;

    // Slippery counter (for ice-patch lanes)
    float m_slipTimer = 0.0f;
};

}  // namespace LightsOut
