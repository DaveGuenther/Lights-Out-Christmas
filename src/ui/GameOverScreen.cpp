#include "ui/GameOverScreen.h"
#include "core/Game.h"
#include <SDL2/SDL.h>
#include <string>
#include <cmath>
#include <random>

namespace LightsOut {

GameOverScreen::GameOverScreen(Game& game, int finalScore, float darknessPct)
    : Screen(game)
    , m_finalScore(finalScore)
    , m_darknessPct(darknessPct)
{}

void GameOverScreen::handleInput() {
    auto& input = m_game.input();
    if (m_timer > 1.0f &&
        (input.isActionJustPressed(Action::MenuConfirm) ||
         input.isActionJustPressed(Action::Bite))) {
        m_accepted = true;
        m_game.resetProgress();
        m_game.replaceState(GameState::MainMenu);
    }
}

void GameOverScreen::update(float dt) {
    m_timer += dt;
}

void GameOverScreen::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    drawBackground(r);
    drawStats(r);
}

void GameOverScreen::drawBackground(SDL_Renderer* r) const {
    // Dark overlay — the neighborhood got you
    SDL_SetRenderDrawColor(r, 5, 5, 15, 255);
    SDL_Rect bg = {0, 0, RENDER_WIDTH, RENDER_HEIGHT};
    SDL_RenderFillRect(r, &bg);

    // A few scattered lights still on — your failure
    std::mt19937 rng(0xF00D);
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(RENDER_WIDTH));
    std::uniform_real_distribution<float> yDist(60.0f, static_cast<float>(RENDER_HEIGHT) - 40.0f);
    static const Color lightColors[] = {{220,60,60},{60,200,60},{60,100,220},{255,220,50}};
    for (int i = 0; i < 12; ++i) {
        int ci = i % 4;
        float lx = xDist(rng), ly = yDist(rng);
        float pulse = 0.5f + 0.5f * std::sin(m_timer * 2.0f + i);
        uint8_t a = static_cast<uint8_t>(60 * pulse);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, lightColors[ci].r, lightColors[ci].g,
                               lightColors[ci].b, a);
        SDL_FRect dot = {lx, ly, 3.0f, 3.0f};
        SDL_RenderFillRectF(r, &dot);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}

void GameOverScreen::drawStats(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();

    float alpha = std::min(1.0f, m_timer * 1.5f);
    uint8_t a = static_cast<uint8_t>(alpha * 255.0f);

    renderer.drawText("CAUGHT!", {RENDER_WIDTH * 0.5f, 30.0f},
                       {200, 50, 50, a}, 8, true);

    renderer.drawText("FINAL SCORE",
                       {RENDER_WIDTH * 0.5f, 55.0f}, {160, 160, 180, a}, 8, true);
    renderer.drawText(std::to_string(m_finalScore),
                       {RENDER_WIDTH * 0.5f, 72.0f}, {255, 220, 50, a}, 8, true);

    int pct = static_cast<int>(m_darknessPct * 100.0f);
    renderer.drawText("DARKNESS",
                       {RENDER_WIDTH * 0.5f, 92.0f}, {160, 160, 180, a}, 8, true);
    renderer.drawText(std::to_string(pct) + "%",
                       {RENDER_WIDTH * 0.5f, 109.0f},
                       {static_cast<uint8_t>(pct * 2), 50, static_cast<uint8_t>(200 - pct), a},
                       8, true);

    if (m_timer > 1.0f) {
        float flash = 0.6f + 0.4f * std::sin(m_timer * 4.0f);
        uint8_t fb = static_cast<uint8_t>(200.0f * flash);
        renderer.drawText("PRESS SPACE TO CONTINUE",
                          {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 15.0f},
                          {fb, fb, fb}, 8, true);
    }
}

}  // namespace LightsOut
