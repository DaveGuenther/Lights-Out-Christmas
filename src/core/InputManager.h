#pragma once
#include "core/Types.h"
#include <SDL2/SDL.h>
#include <array>
#include <string>

namespace LightsOut {

// Number of rebindable gameplay actions (everything before MenuConfirm)
static constexpr int REBINDABLE_ACTION_COUNT = static_cast<int>(Action::Pause) + 1;
static constexpr int ACTION_COUNT            = static_cast<int>(Action::Count);

class InputManager {
public:
    InputManager() = default;
    ~InputManager();

    bool init();
    void shutdown();

    // Persistence: load/save bindings to a JSON file.
    // If load fails, defaults are applied automatically.
    void loadBindings(const std::string& path);
    void saveBindings(const std::string& path) const;

    // Call once per frame before processing events
    void beginFrame();

    // Process a single SDL event
    void handleEvent(const SDL_Event& event);

    // Query action state
    bool isActionDown(Action a)         const;
    bool isActionJustPressed(Action a)  const;
    bool isActionJustReleased(Action a) const;

    // Rebinding — only valid for actions 0..REBINDABLE_ACTION_COUNT-1.
    // Pass SDL_SCANCODE_UNKNOWN / SDL_CONTROLLER_BUTTON_INVALID to clear.
    void rebindKey(Action a, SDL_Scancode sc);
    void rebindPad(Action a, SDL_GameControllerButton btn);

    // Human-readable names for display in the controls screen
    std::string keyNameForAction(Action a)  const;
    std::string padNameForAction(Action a)  const;

    // Raw current key binding values (for controls screen)
    SDL_Scancode               keyBindingFor(Action a)  const;
    SDL_GameControllerButton   padBindingFor(Action a)  const;

    // Gamepad connected?
    bool hasGamepad() const { return m_gamepad != nullptr; }

    bool isKeyDown(SDL_Scancode sc) const;

    // Last scancode / button pressed this frame (reset each beginFrame)
    SDL_Scancode               lastPressedScancode() const { return m_lastPressedScancode; }
    SDL_GameControllerButton   lastPressedButton()   const { return m_lastPressedButton; }

    // Apply factory defaults
    void resetDefaults();

private:
    using ActionArray = std::array<bool, ACTION_COUNT>;
    ActionArray m_actionDown     = {};
    ActionArray m_prevActionDown = {};

    // Rebindable per-action bindings
    std::array<SDL_Scancode,             ACTION_COUNT> m_keyBindings = {};
    std::array<SDL_GameControllerButton, ACTION_COUNT> m_padBindings = {};

    SDL_GameController* m_gamepad = nullptr;

    SDL_Scancode             m_lastPressedScancode = SDL_SCANCODE_UNKNOWN;
    SDL_GameControllerButton m_lastPressedButton   = SDL_CONTROLLER_BUTTON_INVALID;

    void setAction(Action a, bool down);
    void applyKeyEvent(SDL_Scancode sc, bool down);
    void applyPadButton(SDL_GameControllerButton btn, bool down);
};

}  // namespace LightsOut
