#pragma once
#include "PowerUp.h"

namespace LightsOut {

class IcePatch : public PowerUp {
public:
    IcePatch(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "ICE PATCH"; }
    float       duration() const override { return POWERUP_DURATION_ICE; }
};

}  // namespace LightsOut
