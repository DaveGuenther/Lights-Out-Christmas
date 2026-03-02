#pragma once
#include "Threat.h"

namespace LightsOut {

// Slow patrol car — if the player is on the ground when it passes, game over.
// Scrolls from right to left at a constant speed.
class NeighborhoodWatch : public Threat {
public:
    NeighborhoodWatch(float worldX);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    // Returns true if the player is on ground and within the car's detection zone
    bool detectsGroundPlayer(const Rect& playerBounds) const;

private:
    static constexpr float SPOTLIGHT_WIDTH = 40.0f;
    static constexpr float SPOTLIGHT_HEIGHT = 30.0f;
    float m_scrollX;  // car moves with world, but also has own drift
};

}  // namespace LightsOut
