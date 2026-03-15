#include "core/DevConsole.h"
#include "core/Game.h"
#include "core/Constants.h"
#include "core/Types.h"
#include <cctype>

namespace LightsOut {

DevConsole::DevConsole(Game& game) : m_game(game) {}

void DevConsole::toggle() {
    m_open = !m_open;
    if (m_open) {
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
        m_input.clear();
    }
}

void DevConsole::handleEvent(const SDL_Event& event) {
    if (!m_open) return;

    if (event.type == SDL_TEXTINPUT) {
        // Suppress the backtick that opened the console
        if (m_input.empty() && event.text.text[0] == '`') return;
        m_input += event.text.text;
    } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_BACKSPACE:
            if (!m_input.empty()) m_input.pop_back();
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            executeCommand(m_input);
            m_input.clear();
            break;
        case SDL_SCANCODE_ESCAPE:
            toggle();
            break;
        default:
            break;
        }
    }
}

static std::string toUpper(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

void DevConsole::executeCommand(const std::string& raw) {
    std::string cmd   = trim(raw);
    if (cmd.empty()) return;
    std::string upper = toUpper(cmd);

    // ── load <LEVEL_CONSTANT> ─────────────────────────────────────────────────
    if (upper.size() > 5 && upper.substr(0, 5) == "LOAD ") {
        std::string levelName = trim(toUpper(cmd.substr(5)));

        int target = -1;
        if      (levelName == "LEVEL_SUBURBAN")      target = LEVEL_SUBURBAN;
        else if (levelName == "LEVEL_RICH")          target = LEVEL_RICH;
        else if (levelName == "LEVEL_CULDESAC")      target = LEVEL_CULDESAC;
        else if (levelName == "LEVEL_CHRISTMAS_EVE") target = LEVEL_CHRISTMAS_EVE;
        else if (levelName == "LEVEL_TOWN_SQUARE")   target = LEVEL_TOWN_SQUARE;
        else if (levelName == "LEVEL_ENDGAME")       target = LEVEL_ENDGAME;

        if (target >= 0) {
            m_game.setCurrentLevel(target);
            if (target >= LEVEL_ENDGAME) {
                m_game.replaceState(GameState::Endgame);
            } else {
                m_game.replaceState(GameState::Playing);
            }
            m_message = "Loaded " + levelName;
            toggle();  // close console after successful load
        } else {
            m_message = "Unknown level: " + levelName;
        }
        return;
    }

    m_message = "Unknown command: " + cmd;
}

void DevConsole::render() {
    if (!m_open) return;

    auto& ren     = m_game.renderer();
    SDL_Renderer* sdl = ren.sdl();

    // Semi-transparent background strip at bottom of render space
    // Two rows of 14px glyphs + 3px gap between + 2px top padding = 33px total
    SDL_SetRenderDrawBlendMode(sdl, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 200);
    SDL_FRect bg = {0.0f,
                    static_cast<float>(RENDER_HEIGHT) - 35.0f,
                    static_cast<float>(RENDER_WIDTH),
                    35.0f};
    SDL_RenderFillRectF(sdl, &bg);
    SDL_SetRenderDrawBlendMode(sdl, SDL_BLENDMODE_NONE);

    // Previous command result (one row above input, 14px glyph + 3px gap)
    if (!m_message.empty()) {
        ren.drawText(m_message,
                     {3.0f, static_cast<float>(RENDER_HEIGHT) - 31.0f},
                     {160, 220, 160});
    }

    // Input line with cursor — bottom of glyph sits 1px above screen edge
    ren.drawText("> " + m_input + "_",
                 {3.0f, static_cast<float>(RENDER_HEIGHT) - 15.0f},
                 {255, 240, 80});
}

}  // namespace LightsOut
