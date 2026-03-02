#include "core/Game.h"
#include <SDL2/SDL.h>
#include <cstdio>

// SDL_main macro redirects WinMain → main on Windows
#ifdef _WIN32
#include <SDL2/SDL_main.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    LightsOut::Game game;

    if (!game.init("Lights Out Christmas")) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Startup Error",
            "Failed to initialize Lights Out Christmas.\n"
            "Check the console for details.",
            nullptr);
        return 1;
    }

    game.run();
    game.shutdown();
    return 0;
}
