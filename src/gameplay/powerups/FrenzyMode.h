#pragma once
#include "PowerUp.h"

namespace LightsOut {

class FrenzyMode : public PowerUp {
public:
    FrenzyMode(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "FRENZY MODE"; }
    float       duration() const override { return POWERUP_DURATION_FRENZY; }
};

}  // namespace LightsOut
