#pragma once
#include "gameplay/Entity.h"
#include "core/Types.h"

namespace LightsOut {

class Player;

// Base class for all threats
class Threat : public Entity {
public:
    explicit Threat(ThreatType type);
    virtual ~Threat() = default;

    // Called each frame with the player's current position and state
    virtual void update(float dt) override;
    virtual void render(SDL_Renderer* renderer, float cameraX) override;

    // Alert this threat (start chasing the player)
    virtual void alert(const Vec2& playerPos);

    // Freeze this threat for `seconds` (IcePatch power-up)
    void freeze(float seconds);

    // Distract this threat (DecoyNut power-up) — goes to decoy position
    virtual void distract(const Vec2& decoyPos);

    ThreatType threatType() const { return m_type; }
    bool       isAlerted()  const { return m_alerted; }
    bool       isFrozen()   const { return m_frozenTimer > 0.0f; }
    bool       isDistracted() const { return m_distracted; }

    // Returns true when threat has caught the player (game-over condition)
    bool caughtPlayer(const Rect& playerBounds) const;

protected:
    ThreatType m_type;
    bool       m_alerted     = false;
    float      m_frozenTimer = 0.0f;
    bool       m_distracted  = false;
    Vec2       m_targetPos;
    float      m_speed       = 30.0f;
    float      m_alertRadius = 0.0f;

    // Chase-then-give-up behavior
    float m_chaseElapsed  = 0.0f;
    float m_chaseDuration = 0.0f;  // seconds to chase before giving up; 0 = infinite
    bool  m_falling       = false;
    float m_fallVelocity  = 0.0f;

    // Draw a simple colored box representing the threat
    virtual void drawSimple(SDL_Renderer* renderer, float screenX, Color c) const;
};

}  // namespace LightsOut
