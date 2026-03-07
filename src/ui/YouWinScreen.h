#pragma once
#include "core/Game.h"

namespace LightsOut {

class YouWinScreen : public Screen {
public:
    explicit YouWinScreen(Game& game);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    float m_timer = 0.0f;

    void drawBackground(SDL_Renderer* r) const;
    void drawContent(SDL_Renderer* r) const;
};

}  // namespace LightsOut
