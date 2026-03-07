#include "core/InputManager.h"
#include <SDL2/SDL.h>

namespace LightsOut {

InputManager::~InputManager() {
    shutdown();
}

bool InputManager::init() {
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

void InputManager::beginFrame() {
    m_prevActionDown = m_actionDown;
}

void InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        Action a;
        if (scancodeToAction(event.key.keysym.scancode, a)) {
            setAction(a, event.type == SDL_KEYDOWN);
        }
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP: {
        Action a;
        auto btn = static_cast<SDL_GameControllerButton>(event.cbutton.button);
        if (gamepadButtonToAction(btn, a)) {
            setAction(a, event.type == SDL_CONTROLLERBUTTONDOWN);
        }
        break;
    }
    case SDL_CONTROLLERAXISMOTION: {
        auto axis  = static_cast<SDL_GameControllerAxis>(event.caxis.axis);
        Sint16 val = event.caxis.value;
        if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
            setAction(Action::MoveUp,    val < -8000);
            setAction(Action::MoveDown,  val >  8000);
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
    default:
        break;
    }
}

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

bool InputManager::scancodeToAction(SDL_Scancode sc, Action& out) const {
    switch (sc) {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
        out = Action::MoveUp;    return true;
    case SDL_SCANCODE_S:
    case SDL_SCANCODE_DOWN:
        out = Action::MoveDown;  return true;
    case SDL_SCANCODE_A:
    case SDL_SCANCODE_LEFT:
        out = Action::MoveLeft;  return true;
    case SDL_SCANCODE_D:
    case SDL_SCANCODE_RIGHT:
        out = Action::MoveRight; return true;
    case SDL_SCANCODE_SPACE:
    case SDL_SCANCODE_Z:
        out = Action::Bite;      return true;
    case SDL_SCANCODE_X:
    case SDL_SCANCODE_LCTRL:
        out = Action::UsePowerUp; return true;
    case SDL_SCANCODE_ESCAPE:
    case SDL_SCANCODE_P:
        out = Action::Pause;     return true;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        out = Action::MenuConfirm; return true;
    default:
        return false;
    }
}

bool InputManager::gamepadButtonToAction(SDL_GameControllerButton btn, Action& out) const {
    switch (btn) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        out = Action::MoveUp;      return true;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        out = Action::MoveDown;    return true;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        out = Action::MoveLeft;    return true;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        out = Action::MoveRight;   return true;
    case SDL_CONTROLLER_BUTTON_A:
        out = Action::Bite;        return true;
    case SDL_CONTROLLER_BUTTON_B:
        out = Action::UsePowerUp;  return true;
    case SDL_CONTROLLER_BUTTON_START:
        out = Action::Pause;       return true;
    case SDL_CONTROLLER_BUTTON_BACK:
        out = Action::MenuBack;    return true;
    default:
        return false;
    }
}

bool InputManager::gamepadAxisToAction(SDL_GameControllerAxis /*axis*/, Sint16 /*value*/, Action& /*out*/) const {
    // Axis-to-action is handled inline in handleEvent to support proper release.
    return false;
}

}  // namespace LightsOut
