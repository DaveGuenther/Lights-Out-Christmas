#pragma once
#include "core/Types.h"
#include "core/Constants.h"
#include "core/InputManager.h"
#include "core/AudioManager.h"
#include "core/ResourceManager.h"
#include "rendering/Renderer.h"

#include <SDL2/SDL.h>
#include <memory>
#include <stack>
#include <functional>

namespace LightsOut {

class GameWorld;
class Screen;

class Game {
public:
    Game() = default;
    ~Game();

    // Returns false if initialization failed
    bool init(const char* title = "Lights Out Christmas");

    // Run the game loop; returns when window is closed
    void run();

    void shutdown();

    // State management
    void pushState(GameState state);
    void popState();
    void replaceState(GameState state);
    GameState currentState() const;

    // Accessors for subsystems
    InputManager&    input()    { return m_input; }
    AudioManager&    audio()    { return m_audio; }
    ResourceManager& resources(){ return m_resources; }
    Renderer&        renderer() { return m_renderer; }

    // Persistent game data (across levels)
    SquirrelUpgrades& upgrades()         { return m_upgrades; }
    int               totalScore() const  { return m_totalScore; }
    void              addScore(int s)     { m_totalScore += s; }
    int               currentLevel() const { return m_currentLevel; }
    void              nextLevel()         { ++m_currentLevel; }
    void              resetProgress();

    bool shouldQuit() const { return m_quit; }

private:
    void processEvents();
    void update(float dt);
    void render();

    void buildScreenForState(GameState state);

    SDL_Window* m_window  = nullptr;
    bool        m_quit    = false;

    InputManager    m_input;
    AudioManager    m_audio;
    ResourceManager m_resources;
    Renderer        m_renderer;

    // Screen stack — each state owns a Screen
    std::stack<GameState>              m_stateStack;
    std::unique_ptr<Screen>            m_currentScreen;
    GameState                          m_pendingState = GameState::MainMenu;
    bool                               m_stateDirty   = false;

    // Persistent data
    SquirrelUpgrades m_upgrades;
    int              m_totalScore    = 0;
    int              m_currentLevel  = 0;

    // Fixed-timestep accumulator
    float m_accumulator = 0.0f;
    float m_lastTime    = 0.0f;
};

// ─── Abstract Screen base ─────────────────────────────────────────────────────
class Screen {
public:
    explicit Screen(Game& game) : m_game(game) {}
    virtual ~Screen() = default;

    virtual void handleInput() = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;

protected:
    Game& m_game;
};

}  // namespace LightsOut
