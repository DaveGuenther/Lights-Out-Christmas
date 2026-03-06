#include "gameplay/Player.h"
#include "gameplay/LightString.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>
#include <random>

namespace LightsOut {

static float smoothstep(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

Player::Player() : Entity(TAG_PLAYER) {
    position.x = PLAYER_START_X;
    position.y = LANE_GROUND_Y - PLAYER_HEIGHT * 0.5f;
    width      = PLAYER_WIDTH;
    height     = PLAYER_HEIGHT;
    m_currentLane = LaneType::Ground;
    m_targetLane  = LaneType::Ground;
}

void Player::moveUp() {
    if (m_state == PlayerState::Jumping || m_state == PlayerState::Dead) return;
    LaneType target = laneAbove(m_currentLane);
    if (target == m_currentLane) return;

    m_targetLane   = target;
    m_state        = PlayerState::Jumping;
    m_jumpStartY   = position.y;
    m_jumpTargetY  = laneY(target) - height * 0.5f;
    m_jumpProgress = 0.0f;
}

void Player::moveDown() {
    if (m_state == PlayerState::Jumping || m_state == PlayerState::Dead) return;
    LaneType target = laneBelow(m_currentLane);
    if (target == m_currentLane) return;

    m_targetLane   = target;
    m_state        = PlayerState::Jumping;
    m_jumpStartY   = position.y;
    m_jumpTargetY  = laneY(target) - height * 0.5f;
    m_jumpProgress = 0.0f;
}

void Player::tryBite(const std::vector<std::shared_ptr<LightString>>& nearbyStrings, float cameraX) {
    if (m_state == PlayerState::Dead || m_state == PlayerState::Jumping) return;
    m_state = PlayerState::Biting;
    m_animTimer = 0.0f;

    float playerCenterY = position.y + height * 0.5f;
    float playerScreenX = PLAYER_START_X + width * 0.5f;
    LightString* closest = nullptr;
    float closestDist    = PLAYER_BITE_RANGE * 2.0f;

    for (const auto& ls : nearbyStrings) {
        if (ls->isFullyOff() || ls->alive == false) continue;

        // Convert light world X to screen X for comparison with player screen X
        float lsScreenX = (ls->position.x + ls->width * 0.5f) - cameraX;
        float dx = std::abs(playerScreenX - lsScreenX);
        if (dx > PLAYER_BITE_RANGE) continue;

        // Check Y proximity (within PLAYER_BITE_RANGE vertically)
        float lsY = ls->position.y;
        float dy  = std::abs(playerCenterY - lsY);
        if (dy > PLAYER_BITE_RANGE) continue;

        if (dx < closestDist) {
            closestDist = dx;
            closest     = ls.get();
        }
    }

    if (closest) {
        if (isSuperChomp()) {
            // Keep biting until fully off
            while (!closest->isFullyOff()) closest->bite();
        } else {
            closest->bite();
        }
    }
}

void Player::update(float dt) {
    if (m_state == PlayerState::Dead) return;

    updateTimers(dt);

    if (m_state == PlayerState::Jumping) {
        updateLaneTransition(dt);
    }

    // Biting animation times out
    if (m_state == PlayerState::Biting) {
        m_animTimer += dt;
        if (m_animTimer > 0.25f) {
            m_state     = PlayerState::Idle;
            m_animTimer = 0.0f;
        }
    }

    // Slippage on icy lanes
    if (laneInfo(m_currentLane).slippery && m_slipTimer <= 0.0f) {
        std::uniform_real_distribution<float> slip(0.0f, 1.0f);
        // Occasional random slip — handled at GameWorld level
        m_slipTimer = 3.0f;
    } else {
        m_slipTimer -= dt;
    }

    updateAnimation(dt);
}

void Player::updateLaneTransition(float dt) {
    float jumpSpeed = PLAYER_JUMP_SPEED + m_upgrades.jumpHeight * 30.0f;
    float dist      = std::abs(m_jumpTargetY - m_jumpStartY);
    if (dist < 0.001f) dist = 1.0f;

    m_jumpProgress += dt * jumpSpeed / dist;
    m_jumpProgress  = std::min(m_jumpProgress, 1.0f);

    position.y = m_jumpStartY + (m_jumpTargetY - m_jumpStartY) * smoothstep(m_jumpProgress);

    if (m_jumpProgress >= 1.0f) {
        m_currentLane = m_targetLane;
        position.y    = m_jumpTargetY;
        m_state       = PlayerState::Idle;
    }
}

void Player::updateTimers(float dt) {
    auto tick = [&](float& t) { if (t > 0.0f) t = std::max(0.0f, t - dt); };
    tick(m_speedBoostTimer);
    tick(m_invincibleTimer);
    tick(m_shadowTimer);
    tick(m_superChompTimer);
    tick(m_frenzyTimer);
    tick(m_doubleTailTimer);
    if (m_speedBoostTimer <= 0.0f) m_speedBoostMult = 1.0f;
}

void Player::updateAnimation(float dt) {
    m_animTimer += dt;
    float fps = m_animSpeed;
    if (m_animTimer > 1.0f / fps) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % 4;
    }
}

void Player::respawn() {
    m_state        = PlayerState::Idle;
    m_currentLane  = LaneType::Ground;
    m_targetLane   = LaneType::Ground;
    position.y     = laneY(LaneType::Ground) - height * 0.5f;
    m_jumpProgress = 0.0f;

    // Drop any active power-ups (died, so they're lost)
    m_speedBoostTimer = 0.0f;  m_speedBoostMult = 1.0f;
    m_shadowTimer     = 0.0f;
    m_superChompTimer = 0.0f;
    m_frenzyTimer     = 0.0f;
    m_doubleTailTimer = 0.0f;

    // Brief invincibility so the player can reorient
    m_invincibleTimer = PLAYER_RESPAWN_INVINCIBILITY;
}

void Player::applySpeedBoost(float mult, float dur)  { m_speedBoostMult = mult; m_speedBoostTimer = dur; }
void Player::applyInvincibility(float dur)           { m_invincibleTimer = dur; }
void Player::applyShadowMode(float dur)              { m_shadowTimer = dur; }
void Player::applySuperChomp(float dur)              { m_superChompTimer = dur; }
void Player::applyFrenzyMode(float dur)              { m_frenzyTimer = dur; }
void Player::applyDoubleTail(float dur)              { m_doubleTailTimer = dur; }

float Player::frenzySlowFactor() const {
    return (m_frenzyTimer > 0.0f) ? FRENZY_SLOW_FACTOR : 1.0f;
}

void Player::render(SDL_Renderer* renderer, float /*cameraX*/) {
    // Player is always at fixed screen position PLAYER_START_X
    float screenX = PLAYER_START_X;
    float screenY = position.y;

    if (isShadowMode()) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    // Double tail: draw a ghost copy slightly behind
    if (isDoubleTail()) {
        drawSquirrel(renderer, screenX - 12.0f, screenY + 2.0f, true);
    }

    drawSquirrel(renderer, screenX, screenY, isShadowMode());

    // Invincibility: flashing white outline
    if (isInvincible()) {
        Uint32 ticks = SDL_GetTicks();
        if ((ticks / 100) % 2 == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
            SDL_FRect outline = {screenX - 2.0f, screenY - 2.0f,
                                 width + 4.0f, height + 4.0f};
            SDL_RenderDrawRectF(renderer, &outline);
        }
    }

    renderPowerUpGlow(renderer, screenX);
}

// Returns the sprite name for the current player state and animation frame.
// Squirrel sprites face LEFT in the asset sheet; we flip horizontally so the
// character faces RIGHT (direction of travel, since the world scrolls left).
static const char* squirrelSpriteName(PlayerState state, int animFrame) {
    switch (state) {
    case PlayerState::Dead:    return "squirrel_dead";
    case PlayerState::Stunned: return "squirrel_stunned";
    case PlayerState::Biting:  return "squirrel_bite";
    case PlayerState::Jumping: return "squirrel_jump";
    default: break;
    }
    // Idle / Running / anything else — show run cycle
    static const char* run[] = {
        "squirrel_run_0", "squirrel_run_1", "squirrel_run_2", "squirrel_run_3"
    };
    return run[animFrame % 4];
}

void Player::drawSquirrel(SDL_Renderer* renderer, float x, float y, bool shadow) const {
    const char* spriteName = squirrelSpriteName(m_state, m_animFrame);

    // Determine sprite height to bottom-align over the hitbox
    // (most squirrel sprites are 24x18; dead=26x12, stunned/jump=24x20)
    int sw = 0, sh = 0;
    SpriteRegistry::get(spriteName, &sw, &sh);
    if (sw == 0) sh = 18;  // fallback

    // Bottom-align sprite to entity bottom (position.y + PLAYER_HEIGHT)
    // and center horizontally over the hitbox
    float drawX = x + (PLAYER_WIDTH  - static_cast<float>(sw)) * 0.5f;
    float drawY = y + PLAYER_HEIGHT - static_cast<float>(sh);

    uint8_t alpha = shadow ? 80 : 255;

    // Sprites face left in the sheet — flip to face right (direction of travel)
    SpriteRegistry::draw(renderer, spriteName,
                         drawX, drawY, 0.f, 0.f,
                         alpha, SDL_FLIP_HORIZONTAL);
}

void Player::renderPowerUpGlow(SDL_Renderer* renderer, float screenX) const {
    Color glowColor = {0, 0, 0, 0};
    if (m_speedBoostTimer  > 0.0f) glowColor = {255, 200, 50, 80};
    else if (m_shadowTimer > 0.0f) glowColor = {50, 50, 200, 60};
    else if (m_frenzyTimer > 0.0f) glowColor = {200, 50, 200, 80};
    else if (m_invincibleTimer > 0.0f) glowColor = {200, 200, 50, 80};

    if (glowColor.a > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, glowColor.r, glowColor.g, glowColor.b, glowColor.a);
        SDL_FRect glow = {screenX - 4.0f, position.y - 4.0f,
                          width + 8.0f, height + 8.0f};
        SDL_RenderFillRectF(renderer, &glow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

}  // namespace LightsOut
