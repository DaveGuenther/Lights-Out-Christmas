#pragma once
#include "core/Game.h"
#include "core/InputManager.h"

namespace LightsOut {

enum class ControlsMode { Browsing, AwaitingKey, AwaitingPad };

class ControlsScreen : public Screen {
public:
    explicit ControlsScreen(Game& game);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    // Number of rebindable action rows + 1 "RESET DEFAULTS" row
    static constexpr int RESET_ROW = REBINDABLE_ACTION_COUNT;
    static constexpr int NUM_ROWS  = REBINDABLE_ACTION_COUNT + 1;

    int          m_selectedRow = 0;
    int          m_selectedCol = 0;  // 0 = keyboard, 1 = gamepad
    ControlsMode m_mode        = ControlsMode::Browsing;
    float        m_blinkTimer  = 0.0f;

    static const char* actionLabel(int row);
    void commitAndBack();
};

}  // namespace LightsOut
