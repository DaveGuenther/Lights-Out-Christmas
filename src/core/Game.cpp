#include "core/Game.h"
#include "core/SpriteRegistry.h"
#include "ui/MainMenu.h"
#include "ui/GameScreen.h"
#include "ui/PauseMenu.h"
#include "ui/GameOverScreen.h"
#include "ui/UpgradeScreen.h"
#include "ui/TownSquareBossScreen.h"
#include "ui/YouWinScreen.h"
#include "ui/ControlsScreen.h"

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
    SDL_StopTextInput();  // prevent Steam Deck on-screen keyboard from popping up

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
    SpriteRegistry::init(&m_resources);
    if (!m_input.init()) return false;

    // Resolve the controls binding path using SDL's pref-path
    {
        char* prefPath = SDL_GetPrefPath("LightsOutChristmas", "LightsOutChristmas");
        if (prefPath) {
            m_controlsPath = std::string(prefPath) + "controls.json";
            SDL_free(prefPath);
        }
    }
    m_input.loadBindings(m_controlsPath);

    m_audio.init();  // non-fatal if audio fails

    // Load MP3 music for all gameplay levels (falls back to chiptune if file missing)
    {
        std::string mp3 = m_resources.assetPath("music/sugar_plum_fairy.mp3");
        m_audio.loadMusic(Music::Level1, mp3);
        m_audio.loadMusic(Music::Level2, mp3);
        m_audio.loadMusic(Music::Level3, mp3);
        m_audio.loadMusic(Music::Level4, mp3);
        m_audio.loadMusic(Music::Level5, mp3);
    }

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
        // Toggle dev console with backtick
        if (event.type == SDL_KEYDOWN &&
            event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
            m_devConsole.toggle();
            continue;  // don't pass this key to the game
        }
        // While console is open, feed it input and suppress game input
        if (m_devConsole.isOpen()) {
            m_devConsole.handleEvent(event);
            continue;
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
    m_devConsole.render();
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
    case GameState::TownSquareBoss:  // fallthrough — same screen
    case GameState::Endgame:
        m_currentScreen = std::make_unique<TownSquareBossScreen>(*this);
        break;
    case GameState::YouWin:
        m_currentScreen = std::make_unique<YouWinScreen>(*this);
        break;
    case GameState::Controls:
        m_currentScreen = std::make_unique<ControlsScreen>(*this);
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
