#pragma once
#include "Threat.h"

namespace LightsOut {

// Owl lurks in tree branches — grabs the player if they stay on a branch too long
class Owl : public Threat {
public:
    Owl(float worldX, float worldY);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    // Notify the owl that the player is (or isn't) on this branch
    void playerOnBranch(bool on);

private:
    bool  m_playerPresent = false;
    float m_patienceTimer = 0.0f;
    bool  m_attacking     = false;
};

}  // namespace LightsOut
