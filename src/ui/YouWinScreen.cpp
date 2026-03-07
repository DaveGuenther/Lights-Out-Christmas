#include "ui/YouWinScreen.h"
#include "core/Game.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <string>
#include <cmath>
#include <random>

namespace LightsOut {

YouWinScreen::YouWinScreen(Game& game) : Screen(game) {}

void YouWinScreen::handleInput() {
    auto& input = m_game.input();
    if (m_timer > 2.0f &&
        (input.isActionJustPressed(Action::MenuConfirm) ||
         input.isActionJustPressed(Action::Bite))) {
        m_game.resetProgress();
        m_game.replaceState(GameState::MainMenu);
    }
}

void YouWinScreen::update(float dt) {
    m_timer += dt;
}

void YouWinScreen::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    drawBackground(r);
    drawContent(r);
}

void YouWinScreen::drawBackground(SDL_Renderer* r) const {
    // Deep dark — the whole neighborhood is out
    SDL_SetRenderDrawColor(r, 2, 4, 12, 255);
    SDL_Rect bg = {0, 0, RENDER_WIDTH, RENDER_HEIGHT};
    SDL_RenderFillRect(r, &bg);

    // Stars pulsing in the now-dark sky
    std::mt19937 rng(0xF1F1);
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(RENDER_WIDTH));
    std::uniform_real_distribution<float> yDist(0.0f, LANE_GROUND_Y);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 40; ++i) {
        float sx = xDist(rng), sy = yDist(rng);
        float pulse = 0.4f + 0.6f * std::sin(m_timer * 1.3f + i * 0.8f);
        uint8_t sa = static_cast<uint8_t>(200.0f * pulse);
        SDL_SetRenderDrawColor(r, 220, 220, 255, sa);
        SDL_FRect star = {sx, sy, 1.0f, 1.0f};
        SDL_RenderFillRectF(r, &star);
    }
    // Scattered dead lights — dark bulbs across the "houses"
    rng.seed(0xD1D1);
    for (int i = 0; i < 16; ++i) {
        float lx = xDist(rng);
        float ly = 80.0f + static_cast<float>(i % 4) * 20.0f;
        SDL_SetRenderDrawColor(r, 25, 25, 35, 70);
        SDL_FRect dot = {lx, ly, 2.0f, 2.0f};
        SDL_RenderFillRectF(r, &dot);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void YouWinScreen::drawContent(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();

    float alpha = std::min(1.0f, m_timer * 0.9f);
    uint8_t a = static_cast<uint8_t>(alpha * 255.0f);

    renderer.drawText("LIGHTS OUT!",
                       {RENDER_WIDTH * 0.5f, 28.0f},
                       {255, 220, 50, a}, 8, true);

    renderer.drawText("You plunged the whole",
                       {RENDER_WIDTH * 0.5f, 48.0f}, {160, 200, 255, a}, 8, true);
    renderer.drawText("neighborhood into darkness!",
                       {RENDER_WIDTH * 0.5f, 58.0f}, {160, 200, 255, a}, 8, true);

    renderer.drawText("FINAL SCORE",
                       {RENDER_WIDTH * 0.5f, 80.0f}, {160, 160, 180, a}, 8, true);
    renderer.drawText(std::to_string(m_game.totalScore()),
                       {RENDER_WIDTH * 0.5f, 90.0f}, {255, 220, 50, a}, 8, true);

    if (m_timer > 2.0f) {
        float flash = 0.6f + 0.4f * std::sin(m_timer * 4.0f);
        uint8_t fb = static_cast<uint8_t>(200.0f * flash);
        renderer.drawText("PRESS SPACE TO CONTINUE",
                          {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 15.0f},
                          {fb, fb, fb}, 8, true);
    }
}

}  // namespace LightsOut
