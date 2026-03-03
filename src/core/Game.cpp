#include "core/Game.h"
#include "ui/MainMenu.h"
#include "ui/GameScreen.h"
#include "ui/PauseMenu.h"
#include "ui/GameOverScreen.h"
#include "ui/UpgradeScreen.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

namespace LightsOut {

Game::~Game() {
    shutdown();
}

bool Game::init(const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "IMG_Init PNG failed: %s", IMG_GetError());
    }

    m_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    if (!m_renderer.init(m_window)) return false;
    if (!m_resources.init(m_renderer.sdl())) return false;
    if (!m_input.init()) return false;
    m_audio.init();  // non-fatal if audio fails

    // Start at main menu
    m_stateStack.push(GameState::MainMenu);
    buildScreenForState(GameState::MainMenu);

    m_lastTime = static_cast<float>(SDL_GetPerformanceCounter()) /
                 static_cast<float>(SDL_GetPerformanceFrequency());

    SDL_Log("Lights Out Christmas initialized. Render: %dx%d @ %dx scale.",
            RENDER_WIDTH, RENDER_HEIGHT, PIXEL_SCALE);
    return true;
}

void Game::run() {
    while (!m_quit) {
        float now = static_cast<float>(SDL_GetPerformanceCounter()) /
                    static_cast<float>(SDL_GetPerformanceFrequency());
        float frameTime = now - m_lastTime;
        m_lastTime = now;
        // Clamp to avoid spiral of death
        if (frameTime > FIXED_TIMESTEP * MAX_FRAME_SKIP)
            frameTime = FIXED_TIMESTEP * MAX_FRAME_SKIP;

        processEvents();

        // Handle input ONCE per visual frame, before the physics loop.
        // beginFrame() is called every visual frame in processEvents(), so
        // isActionJustPressed() is valid here regardless of the physics rate.
        // Calling handleInput() inside the fixed-step loop would miss key
        // presses at high refresh rates (144Hz+) because beginFrame() would
        // clear the "just pressed" state before the physics step ever ran.
        if (m_stateDirty) {
            m_stateDirty = false;
            buildScreenForState(currentState());
        }
        if (m_currentScreen) m_currentScreen->handleInput();

        m_accumulator += frameTime;
        while (m_accumulator >= FIXED_TIMESTEP) {
            update(FIXED_TIMESTEP);
            m_accumulator -= FIXED_TIMESTEP;
        }

        render();
    }
}

void Game::processEvents() {
    m_input.beginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            m_quit = true;
        }
        if (event.type == SDL_KEYDOWN &&
            event.key.keysym.scancode == SDL_SCANCODE_ESCAPE &&
            currentState() == GameState::MainMenu) {
            m_quit = true;
        }
        m_input.handleEvent(event);
    }
}

void Game::update(float dt) {
    if (m_stateDirty) {
        m_stateDirty = false;
        buildScreenForState(currentState());
    }
    if (m_currentScreen) {
        m_currentScreen->update(dt);
    }
}

void Game::render() {
    m_renderer.beginFrame();
    if (m_currentScreen) m_currentScreen->render();
    m_renderer.endFrame();
}

void Game::pushState(GameState state) {
    m_stateStack.push(state);
    m_pendingState = state;
    m_stateDirty   = true;
}

void Game::popState() {
    if (m_stateStack.size() > 1) m_stateStack.pop();
    m_pendingState = currentState();
    m_stateDirty   = true;
}

void Game::replaceState(GameState state) {
    while (!m_stateStack.empty()) m_stateStack.pop();
    m_stateStack.push(state);
    m_pendingState = state;
    m_stateDirty   = true;
}

GameState Game::currentState() const {
    return m_stateStack.empty() ? GameState::MainMenu : m_stateStack.top();
}

void Game::buildScreenForState(GameState state) {
    switch (state) {
    case GameState::MainMenu:
        m_currentScreen = std::make_unique<MainMenu>(*this);
        break;
    case GameState::Playing:
        m_currentScreen = std::make_unique<GameScreen>(*this);
        break;
    case GameState::Paused:
        m_currentScreen = std::make_unique<PauseMenu>(*this);
        break;
    case GameState::GameOver:
        m_currentScreen = std::make_unique<GameOverScreen>(*this, m_totalScore, 0.0f);
        break;
    case GameState::Upgrade:
        m_currentScreen = std::make_unique<UpgradeScreen>(*this, 0, m_totalScore);
        break;
    default:
        m_currentScreen = std::make_unique<MainMenu>(*this);
        break;
    }
}

void Game::shutdown() {
    m_currentScreen.reset();

    m_resources.shutdown();
    m_audio.shutdown();
    m_input.shutdown();
    m_renderer.shutdown();

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void Game::resetProgress() {
    m_totalScore   = 0;
    m_currentLevel = 0;
    m_lives        = PLAYER_LIVES;
    m_upgrades     = SquirrelUpgrades{};
}

}  // namespace LightsOut
