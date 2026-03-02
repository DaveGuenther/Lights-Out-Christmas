#pragma once
#include "PowerUp.h"

namespace LightsOut {

class DecoyNut : public PowerUp {
public:
    DecoyNut(float worldX, float worldY);
    void apply(Player& player) override;
    std::string name()     const override { return "DECOY NUT"; }
    float       duration() const override { return 0.0f; }  // instant use
};

}  // namespace LightsOut
