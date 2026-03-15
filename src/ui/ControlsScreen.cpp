#include "ui/ControlsScreen.h"
#include "core/Game.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

static const char* kActionLabels[REBINDABLE_ACTION_COUNT] = {
    "MOVE LEFT",
    "MOVE RIGHT",
    "JUMP",
    "DROP",
    "BITE",
    "USE POWER-UP",
    "PAUSE",
};

const char* ControlsScreen::actionLabel(int row) {
    if (row >= 0 && row < REBINDABLE_ACTION_COUNT)
        return kActionLabels[row];
    return "RESET DEFAULTS";
}

ControlsScreen::ControlsScreen(Game& game) : Screen(game) {}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void ControlsScreen::commitAndBack() {
    m_game.input().saveBindings(m_game.controlsPath());
    m_game.replaceState(GameState::MainMenu);
}

// ─── Input ───────────────────────────────────────────────────────────────────

void ControlsScreen::handleInput() {
    auto& input = m_game.input();

    // ── AwaitingKey: capture next key press ──────────────────────────────────
    if (m_mode == ControlsMode::AwaitingKey) {
        SDL_Scancode sc = input.lastPressedScancode();
        if (sc == SDL_SCANCODE_UNKNOWN) return;   // nothing pressed yet
        m_mode = ControlsMode::Browsing;
        if (sc == SDL_SCANCODE_ESCAPE) {
            // ESC cancels — keep the old binding unchanged
        } else if (sc == SDL_SCANCODE_BACKSPACE || sc == SDL_SCANCODE_DELETE) {
            input.rebindKey(static_cast<Action>(m_selectedRow), SDL_SCANCODE_UNKNOWN);
        } else {
            input.rebindKey(static_cast<Action>(m_selectedRow), sc);
        }
        return;
    }

    // ── AwaitingPad: capture next button press ───────────────────────────────
    if (m_mode == ControlsMode::AwaitingPad) {
        SDL_Scancode sc = input.lastPressedScancode();
        SDL_GameControllerButton btn = input.lastPressedButton();

        if (sc == SDL_SCANCODE_ESCAPE) {
            // ESC cancels — keep the old binding unchanged
            m_mode = ControlsMode::Browsing;
        } else if (btn != SDL_CONTROLLER_BUTTON_INVALID) {
            input.rebindPad(static_cast<Action>(m_selectedRow), btn);
            m_mode = ControlsMode::Browsing;
        }
        return;
    }

    // ── Browsing ─────────────────────────────────────────────────────────────
    if (input.isActionJustPressed(Action::MenuUp)) {
        m_selectedRow = (m_selectedRow - 1 + NUM_ROWS) % NUM_ROWS;
    }
    if (input.isActionJustPressed(Action::MenuDown)) {
        m_selectedRow = (m_selectedRow + 1) % NUM_ROWS;
    }
    if (input.isActionJustPressed(Action::MoveLeft)) {
        m_selectedCol = 0;
    }
    if (input.isActionJustPressed(Action::MoveRight)) {
        m_selectedCol = 1;
    }

    if (input.isActionJustPressed(Action::MenuConfirm)) {
        if (m_selectedRow == RESET_ROW) {
            input.resetDefaults();
        } else if (m_selectedCol == 0) {
            m_mode = ControlsMode::AwaitingKey;
        } else {
            m_mode = ControlsMode::AwaitingPad;
        }
    }

    // BACKSPACE clears the binding for the selected action + column
    if (m_selectedRow < RESET_ROW) {
        SDL_Scancode sc = input.lastPressedScancode();
        if (sc == SDL_SCANCODE_BACKSPACE || sc == SDL_SCANCODE_DELETE) {
            if (m_selectedCol == 0)
                input.rebindKey(static_cast<Action>(m_selectedRow), SDL_SCANCODE_UNKNOWN);
            else
                input.rebindPad(static_cast<Action>(m_selectedRow), SDL_CONTROLLER_BUTTON_INVALID);
        }
    }

    if (input.isActionJustPressed(Action::MenuBack) ||
        input.isActionJustPressed(Action::Pause)) {
        commitAndBack();
    }
}

// ─── Update ──────────────────────────────────────────────────────────────────

void ControlsScreen::update(float dt) {
    m_blinkTimer += dt;
}

// ─── Render ──────────────────────────────────────────────────────────────────

void ControlsScreen::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    auto& renderer  = m_game.renderer();
    auto& input     = m_game.input();

    // Background
    SDL_SetRenderDrawColor(r, 8, 12, 35, 255);
    SDL_RenderClear(r);

    // Title
    renderer.drawText("CONTROLS", {RENDER_WIDTH * 0.5f, 8.0f},
                       {255, 220, 50}, 8, true);

    // Column headers (title glyph is 14px tall, so headers start at 8+14+4=26)
    renderer.drawText("ACTION",   {40.0f,  26.0f}, {120, 120, 150}, 8, false);
    renderer.drawText("KEYBOARD", {242.0f, 26.0f}, {120, 120, 150}, 8, false);
    renderer.drawText("GAMEPAD",  {402.0f, 26.0f}, {120, 120, 150}, 8, false);

    // Separator line (below headers: 26+14+4=44)
    SDL_SetRenderDrawColor(r, 60, 60, 90, 255);
    SDL_RenderDrawLine(r, 10, 44, RENDER_WIDTH - 10, 44);

    float rowH    = 20.0f;  // 14px glyph + 6px gap
    float startY  = 50.0f;

    for (int row = 0; row < NUM_ROWS; ++row) {
        float y      = startY + row * rowH;
        bool  active = (row == m_selectedRow);

        // Row highlight
        if (active) {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 255, 220, 50, 30);
            SDL_FRect hi = {10.0f, y - 1.0f, RENDER_WIDTH - 20.0f, rowH};
            SDL_RenderFillRectF(r, &hi);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }

        Color labelCol = active ? Color{255, 220, 50} : Color{160, 160, 180};

        // Action name
        renderer.drawText(actionLabel(row), {40.0f, y}, labelCol, 8, false);

        if (row == RESET_ROW) {
            // "RESET DEFAULTS" row — no binding columns
            if (active) {
                renderer.drawText("[ PRESS ENTER ]", {242.0f, y},
                                   {200, 200, 80}, 8, false);
            }
            continue;
        }

        // Keyboard column
        {
            bool colSel = active && m_selectedCol == 0;
            bool waiting = colSel && m_mode == ControlsMode::AwaitingKey;

            std::string kName;
            if (waiting) {
                bool blink = std::fmod(m_blinkTimer, 0.6f) < 0.3f;
                kName = blink ? "PRESS KEY..." : "";
            } else {
                kName = input.keyNameForAction(static_cast<Action>(row));
            }

            Color kCol = waiting        ? Color{255, 180, 50}
                       : colSel         ? Color{255, 255, 150}
                       : Color{140, 200, 140};
            if (colSel && !waiting) {
                // Draw selection box around keyboard cell
                SDL_SetRenderDrawColor(r, 255, 220, 50, 255);
                SDL_FRect box = {234.0f, y - 1.0f, 140.0f, rowH};
                SDL_RenderDrawRectF(r, &box);
            }
            renderer.drawText(kName, {242.0f, y}, kCol, 8, false);
        }

        // Gamepad column
        {
            bool colSel = active && m_selectedCol == 1;
            bool waiting = colSel && m_mode == ControlsMode::AwaitingPad;

            std::string pName;
            if (waiting) {
                bool blink = std::fmod(m_blinkTimer, 0.6f) < 0.3f;
                pName = blink ? "PRESS BTN..." : "";
            } else {
                pName = input.padNameForAction(static_cast<Action>(row));
            }

            Color pCol = waiting        ? Color{255, 180, 50}
                       : colSel         ? Color{255, 255, 150}
                       : Color{140, 160, 220};
            if (colSel && !waiting) {
                SDL_SetRenderDrawColor(r, 255, 220, 50, 255);
                SDL_FRect box = {394.0f, y - 1.0f, 140.0f, rowH};
                SDL_RenderDrawRectF(r, &box);
            }
            renderer.drawText(pName, {402.0f, y}, pCol, 8, false);
        }
    }

    // Footer instructions (glyph height = 14px, so strip is 14+4=18px)
    SDL_SetRenderDrawColor(r, 40, 40, 60, 255);
    SDL_FRect footerBg = {0.0f, RENDER_HEIGHT - 18.0f,
                          static_cast<float>(RENDER_WIDTH), 18.0f};
    SDL_RenderFillRectF(r, &footerBg);

    renderer.drawText(
        "ENTER:SET  BKSP:CLR  L/R:COL  ESC:BACK",
        {RENDER_WIDTH * 0.5f, RENDER_HEIGHT - 14.0f},
        {100, 100, 120}, 8, true);
}

}  // namespace LightsOut
