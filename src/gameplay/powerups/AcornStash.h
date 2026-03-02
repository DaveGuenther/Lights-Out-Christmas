#pragma once
#include "PowerUp.h"

namespace LightsOut {

class AcornStash : public PowerUp {
public:
    AcornStash(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "ACORN STASH"; }
    float       duration() const override { return POWERUP_DURATION_ACORN; }
};

}  // namespace LightsOut
