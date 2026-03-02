#pragma once
#include "core/Types.h"
#include <SDL2/SDL.h>
#include <array>

namespace LightsOut {

class InputManager {
public:
    InputManager() = default;
    ~InputManager();

    bool init();
    void shutdown();

    // Call once per frame before processing events
    void beginFrame();

    // Process a single SDL event
    void handleEvent(const SDL_Event& event);

    // Query action state
    bool isActionDown(Action a)     const;   // held this frame
    bool isActionJustPressed(Action a) const; // newly pressed this frame
    bool isActionJustReleased(Action a) const;

    // Gamepad connected?
    bool hasGamepad() const { return m_gamepad != nullptr; }

    // Raw keyboard state for debugging
    bool isKeyDown(SDL_Scancode sc) const;

private:
    using ActionArray = std::array<bool, static_cast<int>(Action::Count)>;
    ActionArray m_actionDown     = {};
    ActionArray m_prevActionDown = {};

    SDL_GameController* m_gamepad = nullptr;

    // Maps SDL scancodes / gamepad buttons to Actions
    bool scancodeToAction(SDL_Scancode sc, Action& out) const;
    bool gamepadButtonToAction(SDL_GameControllerButton btn, Action& out) const;
    bool gamepadAxisToAction(SDL_GameControllerAxis axis, Sint16 value, Action& out) const;

    void setAction(Action a, bool down);
};

}  // namespace LightsOut
