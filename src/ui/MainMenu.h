#pragma once
#include "core/Game.h"
#include <vector>
#include <string>

namespace LightsOut {

class MainMenu : public Screen {
public:
    explicit MainMenu(Game& game);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    struct MenuItem {
        std::string label;
        GameState   nextState = GameState::MainMenu;  // ignored when isQuit
        bool        isQuit    = false;
    };

    std::vector<MenuItem> m_items;
    int   m_selected   = 0;
    float m_titleTimer = 0.0f;  // for title animation
    float m_snowTimer  = 0.0f;

    // Simple snow particles on the menu
    struct SnowFlake { float x, y, speed; };
    std::vector<SnowFlake> m_snow;

    SDL_Texture* m_titleTex = nullptr;  // assets/lights_out_title.png

    void drawMenuItems(SDL_Renderer* r) const;
    void drawSnow(SDL_Renderer* r) const;
    void drawControls(SDL_Renderer* r) const;
};

}  // namespace LightsOut
