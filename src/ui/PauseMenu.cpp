#include "ui/PauseMenu.h"
#include "core/Game.h"
#include <SDL2/SDL.h>

namespace LightsOut {

PauseMenu::PauseMenu(Game& game) : Screen(game) {
    m_items = {
        {"RESUME",    [this]() { m_game.popState(); }},
        {"MAIN MENU", [this]() { m_game.replaceState(GameState::MainMenu);
                                  m_game.resetProgress(); }},
    };
}

void PauseMenu::handleInput() {
    auto& input = m_game.input();

    if (input.isActionJustPressed(Action::Pause) ||
        input.isActionJustPressed(Action::MenuBack)) {
        m_game.popState();
        return;
    }
    if (input.isActionJustPressed(Action::MenuUp)) {
        m_selected = (m_selected - 1 + static_cast<int>(m_items.size())) %
                     static_cast<int>(m_items.size());
    }
    if (input.isActionJustPressed(Action::MenuDown)) {
        m_selected = (m_selected + 1) % static_cast<int>(m_items.size());
    }
    if (input.isActionJustPressed(Action::MenuConfirm) ||
        input.isActionJustPressed(Action::Bite)) {
        m_items[m_selected].action();
    }
}

void PauseMenu::update(float /*dt*/) {}

void PauseMenu::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    drawOverlay(r);
    drawItems(r);
}

void PauseMenu::drawOverlay(SDL_Renderer* r) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_Rect overlay = {0, 0, RENDER_WIDTH, RENDER_HEIGHT};
    SDL_RenderFillRect(r, &overlay);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    m_game.renderer().drawText("PAUSED",
        {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.3f},
        {255, 220, 50}, 8, true);
}

void PauseMenu::drawItems(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();
    float startY = RENDER_HEIGHT * 0.5f;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        Color col = (i == m_selected) ? Color{255, 220, 50} : Color{180, 180, 200};
        if (i == m_selected) {
            renderer.drawText(">", {RENDER_WIDTH * 0.5f - 88.0f, startY + i * 17.0f}, col);
        }
        renderer.drawText(m_items[i].label,
                          {RENDER_WIDTH * 0.5f, startY + i * 17.0f}, col, 8, true);
    }
}

}  // namespace LightsOut
