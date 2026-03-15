cmake_minimum_required(VERSION 3.20)
project(LightsOutChristmas VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce clang++ as the compiler
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "This project is intended to be compiled with clang++. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Platform-specific settings
if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(PLATFORM_LINUX)
    # Steam Deck is Linux
    add_compile_definitions(STEAM_DECK_SUPPORT)
endif()

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer")
    if(WIN32)
        # Windows dev machine: optimise for the exact CPU we build on
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
    else()
        # Linux / Steam Deck: target Zen 2 (all Deck hardware) without locking
        # to the compile host's CPU so the binary runs on every Deck revision.
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=znver2")
    endif()
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─── SDL2 Dependencies ───────────────────────────────────────────────────────
# Search order:
#   1. Explicit -DCMAKE_PREFIX_PATH or vcpkg toolchain (user override / CI)
#   2. MSYS2 mingw64  (Windows, auto-detected)
#   3. CMake CONFIG files (vcpkg, MSYS2, Ubuntu 22.04+, Arch, Homebrew)
#   4. PkgConfig      (older Linux distros that ship no CMake config files)
#
# To use a custom SDL2 location on any platform:
#   cmake -DCMAKE_PREFIX_PATH=/path/to/sdl2 ...

# --- Windows: probe MSYS2 when no toolchain file is in use -------------------
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    foreach(_msys_root
            "$ENV{MSYS2_ROOT}/mingw64"
            "C:/msys64/mingw64"
            "C:/msys2/mingw64")
        if(EXISTS "${_msys_root}")
            list(APPEND CMAKE_PREFIX_PATH "${_msys_root}")
            message(STATUS "SDL2: using MSYS2 prefix ${_msys_root}")
            break()
        endif()
    endforeach()
endif()

# --- Attempt CONFIG mode (preferred on all platforms) ------------------------
find_package(SDL2       QUIET CONFIG)
find_package(SDL2_image QUIET CONFIG NAMES SDL2_image sdl2_image)
find_package(SDL2_mixer QUIET CONFIG NAMES SDL2_mixer sdl2_mixer)
find_package(SDL2_ttf   QUIET CONFIG NAMES SDL2_ttf   sdl2_ttf)

# --- PkgConfig fallback for Linux distros without CMake config files ---------
# (Ubuntu < 22.04, Debian Bullseye, older Fedora/CentOS, etc.)
if(UNIX AND NOT WIN32)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        # Helper: if CONFIG mode missed a package, try pkgconfig and create
        # a namespaced INTERFACE target so the rest of the file is unchanged.
        # Requires CMake >= 3.18 for ALIAS of IMPORTED targets (we need 3.20).
        macro(_sdl2_pkgconfig_fallback PKG_VAR PC_NAME NS_TARGET)
            if(NOT ${PKG_VAR}_FOUND)
                pkg_check_modules(${PKG_VAR} QUIET IMPORTED_TARGET ${PC_NAME})
                if(${PKG_VAR}_FOUND AND NOT TARGET ${NS_TARGET})
                    add_library(${NS_TARGET} ALIAS PkgConfig::${PKG_VAR})
                    message(STATUS "SDL2: ${PKG_VAR} found via PkgConfig (${PC_NAME})")
                endif()
            endif()
        endmacro()
        _sdl2_pkgconfig_fallback(SDL2       sdl2       SDL2::SDL2)
        _sdl2_pkgconfig_fallback(SDL2_image SDL2_image SDL2_image::SDL2_image)
        _sdl2_pkgconfig_fallback(SDL2_mixer SDL2_mixer SDL2_mixer::SDL2_mixer)
        _sdl2_pkgconfig_fallback(SDL2_ttf   SDL2_ttf   SDL2_ttf::SDL2_ttf)
    endif()
endif()

# --- Verify all packages were found ------------------------------------------
foreach(_pkg SDL2 SDL2_image SDL2_mixer SDL2_ttf)
    if(NOT ${_pkg}_FOUND)
        message(FATAL_ERROR
            "${_pkg} not found.\n"
            "  Windows (MSYS2): pacman -S mingw-w64-x86_64-SDL2 "
            "mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer "
            "mingw-w64-x86_64-SDL2_ttf\n"
            "  Ubuntu/Debian:   apt install libsdl2-dev libsdl2-image-dev "
            "libsdl2-mixer-dev libsdl2-ttf-dev\n"
            "  Arch/Manjaro:    pacman -S sdl2 sdl2_image sdl2_mixer sdl2_ttf\n"
            "  Fedora/RHEL:     dnf install SDL2-devel SDL2_image-devel "
            "SDL2_mixer-devel SDL2_ttf-devel\n"
            "  Custom path:     cmake -DCMAKE_PREFIX_PATH=/your/sdl2/root ...")
    endif()
endforeach()

# ─── Optional Steamworks SDK ─────────────────────────────────────────────────
option(USE_STEAM "Enable Steamworks SDK integration" OFF)
if(USE_STEAM)
    set(STEAMWORKS_SDK_PATH "" CACHE PATH "Path to Steamworks SDK")
    if(STEAMWORKS_SDK_PATH)
        add_compile_definitions(STEAM_ENABLED)
        if(WIN32)
            set(STEAM_LIB ${STEAMWORKS_SDK_PATH}/redistributable_bin/win64/steam_api64.lib)
            set(STEAM_DLL ${STEAMWORKS_SDK_PATH}/redistributable_bin/win64/steam_api64.dll)
        else()
            set(STEAM_LIB ${STEAMWORKS_SDK_PATH}/redistributable_bin/linux64/libsteam_api.so)
        endif()
        include_directories(${STEAMWORKS_SDK_PATH}/public)
    endif()
endif()

# ─── Game Source Files ────────────────────────────────────────────────────────
set(CORE_SOURCES
    src/core/Game.cpp
    src/core/InputManager.cpp
    src/core/AudioManager.cpp
    src/core/ChiptunePlayer.cpp
    src/core/ResourceManager.cpp
    src/core/SpriteRegistry.cpp
    src/core/HouseAssetLoader.cpp
    src/core/PlatformData.cpp
    src/core/DevConsole.cpp
)

set(GAMEPLAY_SOURCES
    src/gameplay/Entity.cpp
    src/gameplay/Player.cpp
    src/gameplay/Lane.cpp
    src/gameplay/LightString.cpp
    src/gameplay/House.cpp
    src/gameplay/BushTree.cpp
    src/gameplay/GameWorld.cpp
    src/gameplay/ScoreSystem.cpp
    src/gameplay/Collision.cpp
    src/gameplay/DarknessManager.cpp
    src/gameplay/threats/Threat.cpp
    src/gameplay/threats/Homeowner.cpp
    src/gameplay/threats/Dog.cpp
    src/gameplay/threats/Cat.cpp
    src/gameplay/threats/Owl.cpp
    src/gameplay/threats/NeighborhoodWatch.cpp
    src/gameplay/TreeEntity.cpp
    src/gameplay/powerups/PowerUp.cpp
    src/gameplay/powerups/AcornStash.cpp
    src/gameplay/powerups/WinterCoat.cpp
    src/gameplay/powerups/ShadowMode.cpp
    src/gameplay/powerups/SuperChomp.cpp
    src/gameplay/powerups/DecoyNut.cpp
    src/gameplay/powerups/IcePatch.cpp
    src/gameplay/powerups/DoubleTail.cpp
    src/gameplay/powerups/FrenzyMode.cpp
)

set(RENDERING_SOURCES
    src/rendering/Renderer.cpp
    src/rendering/Particles.cpp
)

set(UI_SOURCES
    src/ui/MainMenu.cpp
    src/ui/GameScreen.cpp
    src/ui/PauseMenu.cpp
    src/ui/GameOverScreen.cpp
    src/ui/UpgradeScreen.cpp
    src/ui/TownSquareBossScreen.cpp
    src/ui/YouWinScreen.cpp
    src/ui/ControlsScreen.cpp
)

set(STEAM_SOURCES
    src/steam/SteamIntegration.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${GAMEPLAY_SOURCES}
    ${RENDERING_SOURCES}
    ${UI_SOURCES}
    ${STEAM_SOURCES}
)

# ─── Main Executable ─────────────────────────────────────────────────────────
add_executable(LightsOutChristmas ${ALL_SOURCES})

target_include_directories(LightsOutChristmas PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
)

# On Windows SDL2main must precede SDL2 in the link order: SDL2main.a
# references symbols (SDL_SetMainReady etc.) that are defined in SDL2.dll.a,
# and the single-pass MinGW linker only searches already-seen archives.
if(WIN32)
    target_link_libraries(LightsOutChristmas PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(LightsOutChristmas PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)

if(USE_STEAM AND STEAMWORKS_SDK_PATH)
    target_link_libraries(LightsOutChristmas PRIVATE ${STEAM_LIB})
endif()

# Copy assets to build directory
add_custom_command(TARGET LightsOutChristmas POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:LightsOutChristmas>/assets
    COMMENT "Copying assets..."
)

# Copy steam_appid.txt for development
add_custom_command(TARGET LightsOutChristmas POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/steam_appid.txt
    $<TARGET_FILE_DIR:LightsOutChristmas>/steam_appid.txt
    COMMENT "Copying steam_appid.txt..."
)

# ─── Tests ───────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)

# ─── Install Rules (for Steam deployment) ────────────────────────────────────
install(TARGETS LightsOutChristmas
    RUNTIME DESTINATION .
)
install(DIRECTORY assets/ DESTINATION assets)
install(FILES steam_appid.txt DESTINATION .)

# ─── Build info ──────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== Lights Out Christmas Build Configuration ===")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard:  C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Steam support: ${USE_STEAM}")
message(STATUS "  Platform:      ${CMAKE_SYSTEM_NAME}")
message(STATUS "================================================")
message(STATUS "")
