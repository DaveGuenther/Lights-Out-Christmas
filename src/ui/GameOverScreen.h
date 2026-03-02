#pragma once
#include "core/Game.h"

namespace LightsOut {

class GameOverScreen : public Screen {
public:
    GameOverScreen(Game& game, int finalScore, float darknessPct);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    int   m_finalScore;
    float m_darknessPct;
    float m_timer      = 0.0f;
    bool  m_accepted   = false;

    void drawBackground(SDL_Renderer* r) const;
    void drawStats(SDL_Renderer* r) const;
};

}  // namespace LightsOut
