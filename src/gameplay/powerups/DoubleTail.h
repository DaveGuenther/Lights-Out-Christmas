#pragma once
#include "PowerUp.h"

namespace LightsOut {

class DoubleTail : public PowerUp {
public:
    DoubleTail(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "DOUBLE TAIL"; }
    float       duration() const override { return POWERUP_DURATION_DOUBLE; }
};

}  // namespace LightsOut
