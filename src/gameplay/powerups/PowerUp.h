#pragma once
#include "gameplay/Entity.h"
#include "core/Types.h"
#include <functional>
#include <string>

namespace LightsOut {

class Player;

// Base class for collectible power-ups in the world
class PowerUp : public Entity {
public:
    explicit PowerUp(PowerUpType type, float worldX, float worldY);
    virtual ~PowerUp() = default;

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    PowerUpType powerUpType() const { return m_type; }
    bool        collected()   const { return m_collected; }

    // Apply effect to player; called on collection
    virtual void apply(Player& player) = 0;

    // Human-readable name for HUD
    virtual std::string name() const = 0;

    // Duration of the effect (0 = instant)
    virtual float duration() const = 0;

    // Callback fired when collected
    std::function<void(PowerUpType)> onCollect;

protected:
    PowerUpType m_type;
    bool        m_collected = false;
    float       m_bobTimer  = 0.0f;   // for idle animation
    Color       m_color;
};

}  // namespace LightsOut
