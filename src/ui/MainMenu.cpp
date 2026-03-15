#include "ui/MainMenu.h"
#include "core/Game.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <random>

namespace LightsOut {

MainMenu::MainMenu(Game& game) : Screen(game) {
    m_items = {
        {"PLAY",     GameState::Playing},
        {"CONTROLS", GameState::Controls},
        {"QUIT",     GameState::MainMenu, true},
    };

    // Load title image (ResourceManager::getTexture prepends the asset path internally)
    m_titleTex = m_game.resources().getTexture("lights_out_title.png");

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

    if (input.isActionJustPressed(Action::MenuUp)) {
        m_selected = (m_selected - 1 + static_cast<int>(m_items.size())) %
                     static_cast<int>(m_items.size());
    }
    if (input.isActionJustPressed(Action::MenuDown)) {
        m_selected = (m_selected + 1) % static_cast<int>(m_items.size());
    }
    if (input.isActionJustPressed(Action::MenuConfirm) ||
        input.isActionJustPressed(Action::Bite)) {
        const auto& item = m_items[m_selected];
        if (item.isQuit)
            m_game.quit();
        else
            m_game.replaceState(item.nextState);
    }
}

void MainMenu::update(float dt) {
    m_snowTimer += dt;

    for (auto& f : m_snow) {
        f.y += f.speed * dt;
        if (f.y > RENDER_HEIGHT) f.y = -2.0f;
    }
}

void MainMenu::render() {
    SDL_Renderer* r = m_game.renderer().sdl();

    // ── Title image stretched to fill the full 640×400 render target ─────────
    if (m_titleTex) {
        SDL_Rect dst = {0, 0, RENDER_WIDTH, RENDER_HEIGHT};
        SDL_RenderCopy(r, m_titleTex, nullptr, &dst);
    } else {
        // Fallback: plain dark background if texture failed to load
        SDL_SetRenderDrawColor(r, 8, 12, 35, 255);
        SDL_RenderClear(r);
    }

    drawSnow(r);
    drawMenuItems(r);
    drawControls(r);
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

void MainMenu::drawMenuItems(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();

    constexpr float startY  = 280.0f;
    constexpr float spacing = 19.0f;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        float y = startY + i * spacing;

        Color col = (i == m_selected)
                    ? Color{255, 220, 50}   // selected: yellow
                    : Color{200, 200, 220}; // unselected: light grey

        // Drop-shadow for readability over the image
        renderer.drawText(m_items[i].label,
                          {RENDER_WIDTH * 0.5f + 1, y + 1},
                          {0, 0, 0}, 8, true);

        if (i == m_selected) {
            renderer.drawText(">",
                              {RENDER_WIDTH * 0.5f - 79.0f, y},
                              col);
        }
        renderer.drawText(m_items[i].label,
                          {RENDER_WIDTH * 0.5f, y},
                          col, 8, true);
    }
}

void MainMenu::drawControls(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();
    // Shadow then text for readability over the image
    renderer.drawText("UP/DOWN: SELECT   SPACE/A: CONFIRM",
                      {RENDER_WIDTH * 0.5f + 1, RENDER_HEIGHT - 13.0f},
                      {0, 0, 0}, 8, true);
    renderer.drawText("UP/DOWN: SELECT   SPACE/A: CONFIRM",
                      {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 14.0f},
                      {100, 100, 120}, 8, true);
}

}  // namespace LightsOut
