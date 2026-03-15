#include "gameplay/Player.h"
#include "gameplay/LightString.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>

namespace LightsOut {

Player::Player() : Entity(TAG_PLAYER) {
    position.x  = PLAYER_START_X;
    position.y  = GROUND_FLOOR_Y - PLAYER_HEIGHT;
    width       = PLAYER_WIDTH;
    height      = PLAYER_HEIGHT;
    m_prevFeetY = position.y + PLAYER_HEIGHT;
    m_currentLane = LaneType::Ground;
    m_isGrounded  = true;
}

// ─── Horizontal movement ──────────────────────────────────────────────────────
void Player::moveHorizontal(float dir, float dt) {
    if (m_state == PlayerState::Dead) return;
    m_facingLeft = (dir < 0.0f);
    m_screenX += dir * PLAYER_HORIZONTAL_SPEED * dt;
    m_screenX  = std::max(PLAYER_SCREEN_X_MIN, std::min(PLAYER_SCREEN_X_MAX, m_screenX));
}

// ─── Jump ─────────────────────────────────────────────────────────────────────
void Player::jump() {
    if (m_state == PlayerState::Dead || m_state == PlayerState::Biting) return;
    if (!m_isGrounded) return;

    m_velocityY  = -JUMP_VELOCITY;
    m_isGrounded = false;
    m_state      = PlayerState::Airborne;
}

// ─── Drop through ─────────────────────────────────────────────────────────────
void Player::drop() {
    if (m_state == PlayerState::Dead) return;
    if (!m_isGrounded) return;
    if (m_currentLane == LaneType::Ground) return;

    m_dropping        = true;
    m_dropTimer       = DROP_IGNORE_DURATION;
    m_dropIgnoreTier  = m_currentLane;
    m_isGrounded      = false;
    m_velocityY       = 40.0f;
    m_state           = PlayerState::Airborne;
}

// ─── Drop timer tick (called by GameWorld) ────────────────────────────────────
void Player::tickDropTimer(float dt) {
    if (!m_dropping) return;
    m_dropTimer -= dt;
    if (m_dropTimer <= 0.0f) {
        m_dropping  = false;
        m_dropTimer = 0.0f;
    }
}

// ─── Platform landing (called by GameWorld resolver) ─────────────────────────
void Player::landOnPlatform(float platformY, LaneType tier) {
    position.y    = platformY - height;
    m_velocityY   = 0.0f;
    m_isGrounded  = true;
    m_currentLane = tier;
    if (m_state == PlayerState::Airborne) m_state = PlayerState::Running;

    if (m_dropping && tier != m_dropIgnoreTier) {
        m_dropping  = false;
        m_dropTimer = 0.0f;
    }
}

// ─── Bite ─────────────────────────────────────────────────────────────────────
void Player::tryBite(const std::vector<std::shared_ptr<LightString>>& nearbyStrings,
                     float cameraX)
{
    if (m_state == PlayerState::Dead || m_state == PlayerState::Airborne) return;
    m_state          = PlayerState::Biting;
    m_animTimer      = 0.0f;
    m_biteTimer      = 0.0f;
    m_biteFrameTimer = 0.0f;
    m_biteFrame      = 0;

    // Player bite zone: horizontal range around player center
    float playerScreenX = m_screenX + width * 0.5f;
    float biteLeft  = playerScreenX - PLAYER_BITE_RANGE;
    float biteRight = playerScreenX + PLAYER_BITE_RANGE;

    LightString* closest = nullptr;
    float closestDist    = std::numeric_limits<float>::max();

    for (const auto& ls : nearbyStrings) {
        if (ls->isFullyOff() || ls->alive == false) continue;
        if (ls->lane() != m_currentLane) continue;

        // Check if player bite zone overlaps [strandLeft, strandRight]
        float strandLeft  = ls->position.x - cameraX;
        float strandRight = strandLeft + ls->width;
        if (biteRight < strandLeft || biteLeft > strandRight) continue;

        // Proximity: closest point on strand to player center
        float clampedX = std::max(strandLeft, std::min(strandRight, playerScreenX));
        float dx = std::abs(playerScreenX - clampedX);
        if (dx < closestDist) {
            closestDist = dx;
            closest     = ls.get();
        }
    }

    if (closest) {
        if (isSuperChomp()) {
            while (!closest->isFullyOff()) {
                if (!closest->bite()) break;
            }
        } else {
            closest->bite();
        }
    }
}

// ─── Update ───────────────────────────────────────────────────────────────────
void Player::update(float dt) {
    if (m_state == PlayerState::Dead) return;

    updateTimers(dt);

    // Apply gravity when airborne
    if (!m_isGrounded) {
        m_velocityY += GRAVITY * dt;
        m_velocityY  = std::min(m_velocityY, GRAVITY);  // cap fall speed
    }

    m_prevFeetY = position.y + height;
    position.y += m_velocityY * dt;

    // Bite animation
    if (m_state == PlayerState::Biting) {
        m_biteTimer      += dt;
        m_biteFrameTimer += dt;
        if (m_biteFrameTimer >= 0.1f) {
            m_biteFrameTimer = 0.0f;
            m_biteFrame      = 1 - m_biteFrame;
        }
        if (m_biteTimer >= 0.2f) {
            m_state          = m_isGrounded ? PlayerState::Running : PlayerState::Airborne;
            m_biteTimer      = 0.0f;
            m_biteFrameTimer = 0.0f;
            m_biteFrame      = 0;
        }
    }

    updateAnimation(dt);
}

void Player::setMoving(bool moving) {
    // Only affects grounded states; airborne/biting/dead manage their own transitions
    if (m_state == PlayerState::Dead     ||
        m_state == PlayerState::Airborne ||
        m_state == PlayerState::Biting   ||
        m_state == PlayerState::Stunned) return;
    m_state = moving ? PlayerState::Running : PlayerState::Idle;
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
    bool idle   = (m_state == PlayerState::Idle);
    float speed = idle ? 3.0f : m_animSpeed;   // idle: 3fps, running: 8fps
    int   frames = idle ? 2 : 4;
    m_animTimer += dt;
    if (m_animTimer > 1.0f / speed) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % frames;
    }
}

// ─── Respawn ──────────────────────────────────────────────────────────────────
void Player::respawn() {
    m_state       = PlayerState::Running;
    m_facingLeft  = false;
    m_currentLane = LaneType::Ground;
    position.y    = GROUND_FLOOR_Y - height;
    m_prevFeetY   = GROUND_FLOOR_Y;
    m_screenX     = PLAYER_START_X;
    m_velocityY   = 0.0f;
    m_isGrounded  = true;
    m_dropping    = false;
    m_dropTimer   = 0.0f;

    m_speedBoostTimer = 0.0f;  m_speedBoostMult = 1.0f;
    m_shadowTimer     = 0.0f;
    m_superChompTimer = 0.0f;
    m_frenzyTimer     = 0.0f;
    m_doubleTailTimer = 0.0f;

    m_biteTimer      = 0.0f;
    m_biteFrameTimer = 0.0f;
    m_biteFrame      = 0;

    m_invincibleTimer = PLAYER_RESPAWN_INVINCIBILITY;
}

// ─── Power-ups ────────────────────────────────────────────────────────────────
void Player::applySpeedBoost(float mult, float dur)  { m_speedBoostMult = mult; m_speedBoostTimer = dur; }
void Player::applyInvincibility(float dur)           { m_invincibleTimer = dur; }
void Player::applyShadowMode(float dur)              { m_shadowTimer = dur; }
void Player::applySuperChomp(float dur)              { m_superChompTimer = dur; }
void Player::applyFrenzyMode(float dur)              { m_frenzyTimer = dur; }
void Player::applyDoubleTail(float dur)              { m_doubleTailTimer = dur; }

float Player::frenzySlowFactor() const {
    return (m_frenzyTimer > 0.0f) ? FRENZY_SLOW_FACTOR : 1.0f;
}

// ─── Rendering ────────────────────────────────────────────────────────────────
void Player::render(SDL_Renderer* renderer, float /*cameraX*/) {
    float screenX = m_screenX;
    float screenY = position.y;

    if (isShadowMode()) SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (isDoubleTail()) drawSquirrel(renderer, screenX - 12.0f, screenY + 2.0f, true);

    drawSquirrel(renderer, screenX, screenY, isShadowMode());

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

static const char* squirrelSpriteName(PlayerState state, int animFrame, int biteFrame) {
    switch (state) {
    case PlayerState::Idle:
        return (animFrame % 2 == 0) ? "squirrel_idle_0" : "squirrel_idle_1";
    case PlayerState::Dead:     return "squirrel_dead";
    case PlayerState::Stunned:  return "squirrel_stunned";
    case PlayerState::Airborne: return "squirrel_jump";
    case PlayerState::Biting:
        return (biteFrame == 0) ? "squirrel_bite_0" : "squirrel_bite_1";
    default: break;
    }
    static const char* run[] = {
        "squirrel_run_0", "squirrel_run_1", "squirrel_run_2", "squirrel_run_3"
    };
    return run[animFrame % 4];
}

void Player::drawSquirrel(SDL_Renderer* renderer, float x, float y, bool shadow) const {
    const char* spriteName = squirrelSpriteName(m_state, m_animFrame, m_biteFrame);

    int sw = 0, sh = 0;
    SpriteRegistry::get(spriteName, &sw, &sh);
    if (sw == 0) sh = 18;

    float drawX = x + (PLAYER_WIDTH  - static_cast<float>(sw)) * 0.5f;
    float drawY = y + PLAYER_HEIGHT - static_cast<float>(sh);

    uint8_t alpha = shadow ? 80 : 255;
    SDL_RendererFlip flip = m_facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SpriteRegistry::draw(renderer, spriteName, drawX, drawY, 0.f, 0.f, alpha, flip);
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
