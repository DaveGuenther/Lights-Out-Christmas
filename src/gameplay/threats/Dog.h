#pragma once
#include "Threat.h"

namespace LightsOut {

class Dog : public Threat {
public:
    Dog(float worldX, float worldY, float chainRadius);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;
    void alert(const Vec2& playerPos) override;
    void distract(const Vec2& decoyPos) override;

    // Dog barks to alert homeowner when player is in range but unreachable
    bool isBarking() const { return m_barking; }
    float barkAlertRadius() const { return m_chainRadius * 2.0f; }

private:
    Vec2  m_anchorPos;    // chained position
    float m_chainRadius;
    bool  m_barking = false;
    float m_barkTimer = 0.0f;
    static constexpr float BARK_INTERVAL = 1.5f;
};

}  // namespace LightsOut
