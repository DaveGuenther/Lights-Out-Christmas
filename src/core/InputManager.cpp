#include "core/InputManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <SDL2/SDL.h>

namespace LightsOut {

// ─── Default bindings ─────────────────────────────────────────────────────────
void InputManager::resetDefaults() {
    // Clear all
    m_keyBindings.fill(SDL_SCANCODE_UNKNOWN);
    m_padBindings.fill(SDL_CONTROLLER_BUTTON_INVALID);

    // Gameplay (rebindable)
    m_keyBindings[static_cast<int>(Action::MoveLeft)]   = SDL_SCANCODE_LEFT;
    m_keyBindings[static_cast<int>(Action::MoveRight)]  = SDL_SCANCODE_RIGHT;
    m_keyBindings[static_cast<int>(Action::Jump)]       = SDL_SCANCODE_SPACE;
    m_keyBindings[static_cast<int>(Action::Drop)]       = SDL_SCANCODE_LSHIFT;
    m_keyBindings[static_cast<int>(Action::Bite)]       = SDL_SCANCODE_F;
    m_keyBindings[static_cast<int>(Action::UsePowerUp)] = SDL_SCANCODE_G;
    m_keyBindings[static_cast<int>(Action::Pause)]      = SDL_SCANCODE_ESCAPE;

    // Menu (hardcoded — not shown in rebind screen)
    m_keyBindings[static_cast<int>(Action::MenuConfirm)] = SDL_SCANCODE_RETURN;
    m_keyBindings[static_cast<int>(Action::MenuBack)]    = SDL_SCANCODE_ESCAPE;
    m_keyBindings[static_cast<int>(Action::MenuUp)]      = SDL_SCANCODE_UP;
    m_keyBindings[static_cast<int>(Action::MenuDown)]    = SDL_SCANCODE_DOWN;

    // Gamepad defaults
    m_padBindings[static_cast<int>(Action::MoveLeft)]   = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    m_padBindings[static_cast<int>(Action::MoveRight)]  = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    m_padBindings[static_cast<int>(Action::Jump)]       = SDL_CONTROLLER_BUTTON_A;
    m_padBindings[static_cast<int>(Action::Drop)]       = SDL_CONTROLLER_BUTTON_B;
    m_padBindings[static_cast<int>(Action::Bite)]       = SDL_CONTROLLER_BUTTON_X;
    m_padBindings[static_cast<int>(Action::UsePowerUp)] = SDL_CONTROLLER_BUTTON_Y;
    m_padBindings[static_cast<int>(Action::Pause)]      = SDL_CONTROLLER_BUTTON_START;
    m_padBindings[static_cast<int>(Action::MenuConfirm)]= SDL_CONTROLLER_BUTTON_A;
    m_padBindings[static_cast<int>(Action::MenuBack)]   = SDL_CONTROLLER_BUTTON_B;
    m_padBindings[static_cast<int>(Action::MenuUp)]     = SDL_CONTROLLER_BUTTON_DPAD_UP;
    m_padBindings[static_cast<int>(Action::MenuDown)]   = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
}

// ─── Init / shutdown ──────────────────────────────────────────────────────────
InputManager::~InputManager() { shutdown(); }

bool InputManager::init() {
    resetDefaults();
    if (SDL_NumJoysticks() > 0) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                m_gamepad = SDL_GameControllerOpen(i);
                if (m_gamepad) {
                    SDL_Log("Gamepad connected: %s", SDL_GameControllerName(m_gamepad));
                    break;
                }
            }
        }
    }
    return true;
}

void InputManager::shutdown() {
    if (m_gamepad) {
        SDL_GameControllerClose(m_gamepad);
        m_gamepad = nullptr;
    }
}

// ─── Persistence ──────────────────────────────────────────────────────────────
static const char* actionNames[static_cast<int>(Action::Count)] = {
    "MoveLeft", "MoveRight", "Jump", "Drop", "Bite", "UsePowerUp", "Pause",
    "MenuConfirm", "MenuBack", "MenuUp", "MenuDown"
};

void InputManager::loadBindings(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    nlohmann::json j;
    try { f >> j; } catch (...) { return; }

    if (j.contains("keyboard") && j["keyboard"].is_object()) {
        for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
            const char* name = actionNames[i];
            if (j["keyboard"].contains(name)) {
                int sc = j["keyboard"][name].get<int>();
                m_keyBindings[i] = static_cast<SDL_Scancode>(sc);
            }
        }
    }
    if (j.contains("gamepad") && j["gamepad"].is_object()) {
        for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
            const char* name = actionNames[i];
            if (j["gamepad"].contains(name)) {
                int btn = j["gamepad"][name].get<int>();
                m_padBindings[i] = static_cast<SDL_GameControllerButton>(btn);
            }
        }
    }
}

void InputManager::saveBindings(const std::string& path) const {
    nlohmann::json j;
    j["keyboard"] = nlohmann::json::object();
    j["gamepad"]  = nlohmann::json::object();
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
        j["keyboard"][actionNames[i]] = static_cast<int>(m_keyBindings[i]);
        j["gamepad"] [actionNames[i]] = static_cast<int>(m_padBindings[i]);
    }
    std::ofstream f(path);
    if (f.is_open()) f << j.dump(2);
}

// ─── Frame / event ────────────────────────────────────────────────────────────
void InputManager::beginFrame() {
    m_prevActionDown      = m_actionDown;
    m_lastPressedScancode = SDL_SCANCODE_UNKNOWN;
    m_lastPressedButton   = SDL_CONTROLLER_BUTTON_INVALID;
}

void InputManager::applyKeyEvent(SDL_Scancode sc, bool down) {
    for (int i = 0; i < ACTION_COUNT; ++i) {
        if (m_keyBindings[i] == sc && sc != SDL_SCANCODE_UNKNOWN) {
            setAction(static_cast<Action>(i), down);
        }
    }
}

void InputManager::applyPadButton(SDL_GameControllerButton btn, bool down) {
    for (int i = 0; i < ACTION_COUNT; ++i) {
        if (m_padBindings[i] == btn && btn != SDL_CONTROLLER_BUTTON_INVALID) {
            setAction(static_cast<Action>(i), down);
        }
    }
}

void InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (event.type == SDL_KEYDOWN)
            m_lastPressedScancode = event.key.keysym.scancode;
        applyKeyEvent(event.key.keysym.scancode, event.type == SDL_KEYDOWN);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        if (event.type == SDL_CONTROLLERBUTTONDOWN)
            m_lastPressedButton = static_cast<SDL_GameControllerButton>(event.cbutton.button);
        applyPadButton(static_cast<SDL_GameControllerButton>(event.cbutton.button),
                       event.type == SDL_CONTROLLERBUTTONDOWN);
        break;
    case SDL_CONTROLLERAXISMOTION: {
        auto axis  = static_cast<SDL_GameControllerAxis>(event.caxis.axis);
        Sint16 val = event.caxis.value;
        if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
            setAction(Action::Jump, val < -8000);
            setAction(Action::Drop, val >  8000);
        } else if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
            setAction(Action::MoveLeft,  val < -8000);
            setAction(Action::MoveRight, val >  8000);
        }
        break;
    }
    case SDL_CONTROLLERDEVICEADDED:
        if (!m_gamepad && SDL_IsGameController(event.cdevice.which)) {
            m_gamepad = SDL_GameControllerOpen(event.cdevice.which);
            if (m_gamepad)
                SDL_Log("Gamepad connected: %s", SDL_GameControllerName(m_gamepad));
        }
        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        if (m_gamepad) {
            SDL_GameControllerClose(m_gamepad);
            m_gamepad = nullptr;
            SDL_Log("Gamepad disconnected");
        }
        break;
    default: break;
    }
}

// ─── Query ────────────────────────────────────────────────────────────────────
bool InputManager::isActionDown(Action a) const {
    return m_actionDown[static_cast<int>(a)];
}
bool InputManager::isActionJustPressed(Action a) const {
    int i = static_cast<int>(a);
    return m_actionDown[i] && !m_prevActionDown[i];
}
bool InputManager::isActionJustReleased(Action a) const {
    int i = static_cast<int>(a);
    return !m_actionDown[i] && m_prevActionDown[i];
}
bool InputManager::isKeyDown(SDL_Scancode sc) const {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    return state[sc] != 0;
}

void InputManager::setAction(Action a, bool down) {
    m_actionDown[static_cast<int>(a)] = down;
}

// ─── Rebinding ────────────────────────────────────────────────────────────────
void InputManager::rebindKey(Action a, SDL_Scancode sc) {
    int ai = static_cast<int>(a);
    if (ai >= REBINDABLE_ACTION_COUNT) return;
    // Clear any other action that had this scancode
    if (sc != SDL_SCANCODE_UNKNOWN) {
        for (int i = 0; i < REBINDABLE_ACTION_COUNT; ++i)
            if (m_keyBindings[i] == sc) m_keyBindings[i] = SDL_SCANCODE_UNKNOWN;
    }
    m_keyBindings[ai] = sc;
}

void InputManager::rebindPad(Action a, SDL_GameControllerButton btn) {
    int ai = static_cast<int>(a);
    if (ai >= REBINDABLE_ACTION_COUNT) return;
    if (btn != SDL_CONTROLLER_BUTTON_INVALID) {
        for (int i = 0; i < REBINDABLE_ACTION_COUNT; ++i)
            if (m_padBindings[i] == btn) m_padBindings[i] = SDL_CONTROLLER_BUTTON_INVALID;
    }
    m_padBindings[ai] = btn;
}

// ─── Display names ────────────────────────────────────────────────────────────
std::string InputManager::keyNameForAction(Action a) const {
    SDL_Scancode sc = m_keyBindings[static_cast<int>(a)];
    if (sc == SDL_SCANCODE_UNKNOWN) return "---";
    const char* name = SDL_GetScancodeName(sc);
    return name ? name : "???";
}

std::string InputManager::padNameForAction(Action a) const {
    SDL_GameControllerButton btn = m_padBindings[static_cast<int>(a)];
    if (btn == SDL_CONTROLLER_BUTTON_INVALID) return "---";
    static const char* btnNames[] = {
        "A", "B", "X", "Y", "Back", "Guide", "Start",
        "L-Stick", "R-Stick", "L-Shoulder", "R-Shoulder",
        "D-Up", "D-Down", "D-Left", "D-Right", "Misc"
    };
    int bi = static_cast<int>(btn);
    if (bi >= 0 && bi < static_cast<int>(std::size(btnNames))) return btnNames[bi];
    return "???";
}

SDL_Scancode InputManager::keyBindingFor(Action a) const {
    return m_keyBindings[static_cast<int>(a)];
}
SDL_GameControllerButton InputManager::padBindingFor(Action a) const {
    return m_padBindings[static_cast<int>(a)];
}

}  // namespace LightsOut
