#pragma once
#include "Threat.h"

namespace LightsOut {

// Cats are sneaky — they don't alert homeowners, but chase the player across
// rooftops and power lines.  They never give up.
class Cat : public Threat {
public:
    Cat(float worldX, float worldY);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

private:
    float m_stalkerTimer = 0.0f;   // time before cat decides to engage
    static constexpr float STALK_DELAY = 2.0f;
    static constexpr float CAT_LANES_BITMASK = 0b00111;  // Rooftop|PowerLine|Branch
};

}  // namespace LightsOut
