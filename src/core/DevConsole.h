#pragma once
#include <string>
#include <SDL2/SDL.h>

namespace LightsOut {

class Game;

class DevConsole {
public:
    explicit DevConsole(Game& game);

    bool isOpen() const { return m_open; }
    void toggle();
    void handleEvent(const SDL_Event& event);
    void render();

private:
    Game&       m_game;
    bool        m_open    = false;
    std::string m_input;
    std::string m_message;  // last command result

    void executeCommand(const std::string& cmd);
};

}  // namespace LightsOut
