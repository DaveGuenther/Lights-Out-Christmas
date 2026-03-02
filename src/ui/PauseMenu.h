#pragma once
#include "core/Game.h"
#include <vector>
#include <string>

namespace LightsOut {

class PauseMenu : public Screen {
public:
    explicit PauseMenu(Game& game);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    struct MenuItem {
        std::string label;
        std::function<void()> action;
    };

    std::vector<MenuItem> m_items;
    int m_selected = 0;

    void drawOverlay(SDL_Renderer* r) const;
    void drawItems(SDL_Renderer* r) const;
};

}  // namespace LightsOut
