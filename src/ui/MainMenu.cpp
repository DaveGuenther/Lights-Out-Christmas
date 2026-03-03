#include "ui/MainMenu.h"
#include "core/Game.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <random>

namespace LightsOut {

MainMenu::MainMenu(Game& game) : Screen(game) {
    m_items = {
        {"PLAY",        GameState::Playing},
        {"LEADERBOARD", GameState::Leaderboard},
    };

    // Init snow
    std::mt19937 rng(SDL_GetTicks());
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(RENDER_WIDTH));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(RENDER_HEIGHT));
    std::uniform_real_distribution<float> sDist(8.0f, 18.0f);
    m_snow.resize(40);
    for (auto& f : m_snow) {
        f.x = xDist(rng);
        f.y = yDist(rng);
        f.speed = sDist(rng);
    }
}

void MainMenu::handleInput() {
    auto& input = m_game.input();

    if (input.isActionJustPressed(Action::MoveUp)) {
        m_selected = (m_selected - 1 + static_cast<int>(m_items.size())) %
                     static_cast<int>(m_items.size());
    }
    if (input.isActionJustPressed(Action::MoveDown)) {
        m_selected = (m_selected + 1) % static_cast<int>(m_items.size());
    }
    if (input.isActionJustPressed(Action::MenuConfirm) ||
        input.isActionJustPressed(Action::Bite)) {
        m_game.replaceState(m_items[m_selected].nextState);
    }
}

void MainMenu::update(float dt) {
    m_titleTimer += dt;
    m_snowTimer  += dt;

    for (auto& f : m_snow) {
        f.y += f.speed * dt;
        if (f.y > RENDER_HEIGHT) f.y = -2.0f;
    }
}

void MainMenu::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    drawBackground(r);
    drawSnow(r);
    drawTitle(r);
    drawMenuItems(r);
    drawControls(r);
}

void MainMenu::drawBackground(SDL_Renderer* r) const {
    // Night sky gradient
    for (int y = 0; y < RENDER_HEIGHT; y += 2) {
        float t = static_cast<float>(y) / RENDER_HEIGHT;
        SDL_SetRenderDrawColor(r,
            static_cast<uint8_t>(8  + t * 12),
            static_cast<uint8_t>(12 + t * 8),
            static_cast<uint8_t>(35 + t * 20), 255);
        SDL_RenderDrawLine(r, 0, y, RENDER_WIDTH, y);
        SDL_RenderDrawLine(r, 0, y+1, RENDER_WIDTH, y+1);
    }

    // Ground
    SDL_SetRenderDrawColor(r, 30, 45, 25, 255);
    SDL_Rect ground = {0, RENDER_HEIGHT - 30, RENDER_WIDTH, 30};
    SDL_RenderFillRect(r, &ground);

    // Snow on ground
    SDL_SetRenderDrawColor(r, 200, 220, 240, 255);
    SDL_Rect snowGround = {0, RENDER_HEIGHT - 30, RENDER_WIDTH, 5};
    SDL_RenderFillRect(r, &snowGround);

    // Some silhouetted houses
    SDL_SetRenderDrawColor(r, 15, 20, 30, 255);
    int houseY = RENDER_HEIGHT - 30;
    for (int i = 0; i < 5; ++i) {
        int hx = i * 65;
        SDL_Rect hwall = {hx, houseY - 35, 55, 35};
        SDL_RenderFillRect(r, &hwall);
        SDL_Rect hroof = {hx - 3, houseY - 45, 61, 14};
        SDL_RenderFillRect(r, &hroof);
    }

    // Stars
    std::mt19937 starRng(0xDEADBEEF);
    std::uniform_int_distribution<int> sxDist(0, RENDER_WIDTH);
    std::uniform_int_distribution<int> syDist(0, 80);
    std::uniform_int_distribution<int> brightDist(100, 220);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 45; ++i) {
        int sx = sxDist(starRng), sy = syDist(starRng);
        uint8_t b = static_cast<uint8_t>(brightDist(starRng));
        SDL_SetRenderDrawColor(r, b, b, b, b);
        SDL_RenderDrawPoint(r, sx, sy);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void MainMenu::drawSnow(SDL_Renderer* r) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 220, 235, 255, 180);
    for (const auto& f : m_snow) {
        SDL_FRect flake = {f.x, f.y, 1.0f, 2.0f};
        SDL_RenderFillRectF(r, &flake);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void MainMenu::drawTitle(SDL_Renderer* r) const {
    // "LIGHTS OUT" on one line, "CHRISTMAS" below
    // Flash effect using title timer
    float pulse = 0.7f + 0.3f * std::sin(m_titleTimer * 3.0f);
    uint8_t brightness = static_cast<uint8_t>(200 * pulse);

    auto& renderer = m_game.renderer();

    // Shadow
    renderer.drawText("LIGHTS OUT", {RENDER_WIDTH * 0.5f + 1, 22}, {30, 30, 30}, 8, true);
    renderer.drawText("CHRISTMAS",  {RENDER_WIDTH * 0.5f + 1, 32}, {30, 30, 30}, 8, true);

    // Main text — red for LIGHTS OUT
    renderer.drawText("LIGHTS OUT", {RENDER_WIDTH * 0.5f, 21},
                       {brightness, 40, 40}, 8, true);
    // Green for CHRISTMAS
    renderer.drawText("CHRISTMAS", {RENDER_WIDTH * 0.5f, 31},
                       {40, static_cast<uint8_t>(brightness * 0.8f), 40}, 8, true);

    // Squirrel icon (simple)
    float sq = RENDER_WIDTH * 0.5f + 50.0f;
    float sy = 24.0f;
    SDL_SetRenderDrawColor(r, 140, 85, 30, 255);
    SDL_FRect sbody = {sq, sy, 10, 8};
    SDL_RenderFillRectF(r, &sbody);
    SDL_FRect shead = {sq + 5, sy - 4, 7, 6};
    SDL_RenderFillRectF(r, &shead);
    SDL_FRect stail = {sq - 3, sy, 4, 8};
    SDL_RenderFillRectF(r, &stail);
}

void MainMenu::drawMenuItems(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();

    float startY = 65.0f;
    float spacing = 14.0f;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        Color col = (i == m_selected)
                    ? Color{255, 220, 50}   // selected: yellow
                    : Color{160, 160, 180}; // unselected: grey

        // Selection indicator
        if (i == m_selected) {
            renderer.drawText(">", {RENDER_WIDTH * 0.5f - 30.0f,
                                    startY + i * spacing}, col);
        }
        renderer.drawText(m_items[i].label,
                          {RENDER_WIDTH * 0.5f, startY + i * spacing},
                          col, 8, true);
    }
}

void MainMenu::drawControls(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();
    renderer.drawText("UP/DOWN: SELECT  SPACE/A: CONFIRM",
                      {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 15.0f},
                      {100, 100, 120}, 8, true);
    renderer.drawText("ESC: QUIT",
                      {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 8.0f},
                      {80, 80, 100}, 8, true);
}

}  // namespace LightsOut
