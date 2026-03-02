#include "ui/UpgradeScreen.h"
#include "core/Game.h"
#include <SDL2/SDL.h>
#include <string>
#include <cmath>

namespace LightsOut {

UpgradeScreen::UpgradeScreen(Game& game, int levelScore, int totalScore)
    : Screen(game)
    , m_levelScore(levelScore)
    , m_totalScore(totalScore)
    , m_spendable(levelScore)
{
    buildOptions();
}

void UpgradeScreen::buildOptions() {
    auto& upg = m_game.upgrades();
    m_options = {
        {UpgradeType::BiteSpeed,    "BITE SPEED",  "Faster chomping through strands",
         UPGRADE_BITE_SPEED_COST,   upg.biteSpeed},
        {UpgradeType::MovementSpeed,"MOVE SPEED",  "Jump between lanes faster",
         UPGRADE_MOVE_SPEED_COST,   upg.moveSpeed},
        {UpgradeType::QuietSteps,   "QUIET STEPS", "Smaller detection radius",
         UPGRADE_QUIET_STEPS_COST,  upg.quietSteps},
        {UpgradeType::JumpHeight,   "JUMP HEIGHT", "Leap two lanes at once",
         UPGRADE_JUMP_HEIGHT_COST,  upg.jumpHeight},
    };
}

bool UpgradeScreen::canAfford(const UpgradeOption& opt) const {
    return m_spendable >= opt.cost &&
           opt.currentLevel < SquirrelUpgrades::MAX_LEVEL;
}

void UpgradeScreen::purchase(const UpgradeOption& opt) {
    if (!canAfford(opt)) return;
    m_spendable -= opt.cost;

    auto& upg = m_game.upgrades();
    switch (opt.type) {
    case UpgradeType::BiteSpeed:    upg.biteSpeed++;    break;
    case UpgradeType::MovementSpeed:upg.moveSpeed++;    break;
    case UpgradeType::QuietSteps:   upg.quietSteps++;   break;
    case UpgradeType::JumpHeight:   upg.jumpHeight++;   break;
    default: break;
    }
    buildOptions();  // refresh levels
}

void UpgradeScreen::handleInput() {
    auto& input = m_game.input();

    if (input.isActionJustPressed(Action::MenuUp)) {
        m_selected = (m_selected - 1 + static_cast<int>(m_options.size())) %
                     static_cast<int>(m_options.size());
    }
    if (input.isActionJustPressed(Action::MenuDown)) {
        m_selected = (m_selected + 1) % static_cast<int>(m_options.size());
    }
    if (input.isActionJustPressed(Action::Bite) ||
        input.isActionJustPressed(Action::MenuConfirm)) {
        if (canAfford(m_options[m_selected])) {
            purchase(m_options[m_selected]);
        }
    }
    // Skip upgrades
    if (input.isActionJustPressed(Action::Pause) ||
        input.isActionJustPressed(Action::MenuBack)) {
        m_game.replaceState(GameState::Playing);
    }
}

void UpgradeScreen::update(float dt) {
    m_timer += dt;
}

void UpgradeScreen::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    drawBackground(r);
    drawTitle(r);
    drawOptions(r);
    drawBudget(r);
    drawSquirrelPreview(r);
}

void UpgradeScreen::drawBackground(SDL_Renderer* r) const {
    SDL_SetRenderDrawColor(r, 10, 15, 30, 255);
    SDL_Rect bg = {0, 0, RENDER_WIDTH, RENDER_HEIGHT};
    SDL_RenderFillRect(r, &bg);
}

void UpgradeScreen::drawTitle(SDL_Renderer* r) const {
    (void)r;
    m_game.renderer().drawText("UPGRADE YOUR SQUIRREL",
                               {RENDER_WIDTH * 0.5f, 8.0f},
                               {255, 220, 50}, 8, true);
    m_game.renderer().drawText("LEVEL COMPLETE!",
                               {RENDER_WIDTH * 0.5f, 18.0f},
                               {100, 220, 100}, 8, true);
}

void UpgradeScreen::drawOptions(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();
    float startY = 38.0f;
    float rowH   = 22.0f;

    for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
        const auto& opt = m_options[i];
        bool    sel      = (i == m_selected);
        bool    afford   = canAfford(opt);
        bool    maxed    = (opt.currentLevel >= SquirrelUpgrades::MAX_LEVEL);

        float y = startY + i * rowH;

        // Selection highlight
        if (sel) {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 255, 220, 50, 30);
            SDL_FRect hl = {10.0f, y - 2.0f, RENDER_WIDTH - 20.0f, rowH - 2.0f};
            SDL_RenderFillRectF(r, &hl);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }

        Color nameCol = maxed ? Color{100, 100, 100}
                      : afford ? Color{255, 220, 50}
                      : Color{140, 140, 140};
        renderer.drawText(opt.name, {15.0f, y}, nameCol);

        // Level pips
        for (int lv = 0; lv < SquirrelUpgrades::MAX_LEVEL; ++lv) {
            SDL_SetRenderDrawColor(r,
                lv < opt.currentLevel ? 100 : 40,
                lv < opt.currentLevel ? 200 : 80,
                lv < opt.currentLevel ? 100 : 40, 255);
            SDL_FRect pip = {RENDER_WIDTH - 40.0f + lv * 9.0f, y + 1.0f, 6.0f, 5.0f};
            SDL_RenderFillRectF(r, &pip);
        }

        // Cost
        if (!maxed) {
            Color costCol = afford ? Color{200, 200, 100} : Color{100, 80, 80};
            renderer.drawText(std::to_string(opt.cost) + "PTS",
                              {RENDER_WIDTH - 62.0f, y + 8.0f}, costCol);
        } else {
            renderer.drawText("MAX", {RENDER_WIDTH - 62.0f, y + 8.0f},
                              {60, 200, 60});
        }

        // Description (small)
        renderer.drawText(opt.description, {15.0f, y + 9.0f},
                          sel ? Color{180, 180, 200} : Color{100, 100, 120});
    }

    renderer.drawText("PRESS ESC TO SKIP",
                      {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 8.0f},
                      {80, 80, 100}, 8, true);
}

void UpgradeScreen::drawBudget(SDL_Renderer* r) const {
    (void)r;
    m_game.renderer().drawText("POINTS: " + std::to_string(m_spendable),
                               {RENDER_WIDTH - 5.0f, 8.0f},
                               {255, 200, 50});
}

void UpgradeScreen::drawSquirrelPreview(SDL_Renderer* r) const {
    // Draw a simple squirrel preview in bottom-right
    float sx = RENDER_WIDTH - 25.0f;
    float sy = RENDER_HEIGHT - 35.0f;

    SDL_SetRenderDrawColor(r, 140, 85, 30, 255);
    SDL_FRect body = {sx, sy + 5, 10, 8};
    SDL_RenderFillRectF(r, &body);
    SDL_FRect head = {sx + 5, sy + 1, 7, 6};
    SDL_RenderFillRectF(r, &head);

    // Upgrade glow based on what's been upgraded
    auto& upg = m_game.upgrades();
    if (upg.biteSpeed > 0 || upg.moveSpeed > 0) {
        float pulse = 0.5f + 0.5f * std::sin(m_timer * 4.0f);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 255, 200, 50, static_cast<uint8_t>(60 * pulse));
        SDL_FRect glow = {sx - 3, sy - 1, 20, 20};
        SDL_RenderFillRectF(r, &glow);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}

}  // namespace LightsOut
