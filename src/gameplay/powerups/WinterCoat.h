#pragma once
#include "PowerUp.h"

namespace LightsOut {

class WinterCoat : public PowerUp {
public:
    WinterCoat(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "WINTER COAT"; }
    float       duration() const override { return POWERUP_DURATION_COAT; }
};

}  // namespace LightsOut
