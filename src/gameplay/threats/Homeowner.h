#pragma once
#include "Threat.h"

namespace LightsOut {

class Homeowner : public Threat {
public:
    enum class Personality { GrumpyOldMan, DadInPajamas };

    Homeowner(float worldX, float worldY, Personality p);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;
    void alert(const Vec2& playerPos) override;

    // GrumpyOldMan: slow but relentless (never gives up)
    // DadInPajamas: fast but gives up after giveUpTime seconds
    bool hasGivenUp() const { return m_givenUp; }

private:
    Personality m_personality;
    float       m_giveUpTimer = 0.0f;
    bool        m_givenUp     = false;
    static constexpr float DAD_GIVE_UP_TIME = 6.0f;
};

}  // namespace LightsOut
