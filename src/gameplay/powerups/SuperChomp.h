#pragma once
#include "PowerUp.h"

namespace LightsOut {

class SuperChomp : public PowerUp {
public:
    SuperChomp(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "SUPER CHOMP"; }
    float       duration() const override { return POWERUP_DURATION_CHOMP; }
};

}  // namespace LightsOut
