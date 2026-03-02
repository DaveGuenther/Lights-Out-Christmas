#pragma once
#include "PowerUp.h"

namespace LightsOut {

class ShadowMode : public PowerUp {
public:
    ShadowMode(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "SHADOW MODE"; }
    float       duration() const override { return POWERUP_DURATION_SHADOW; }
};

}  // namespace LightsOut
