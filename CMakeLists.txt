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
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─── SDL2 Dependencies ───────────────────────────────────────────────────────
find_package(SDL2 REQUIRED CONFIG NAMES SDL2)
find_package(SDL2_image REQUIRED CONFIG NAMES SDL2_image)
find_package(SDL2_mixer REQUIRED CONFIG NAMES SDL2_mixer)
find_package(SDL2_ttf REQUIRED CONFIG NAMES SDL2_ttf)

# Handle different SDL2 CMake target names (vcpkg vs system)
if(TARGET SDL2::SDL2)
    set(SDL2_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(SDL2_TARGET SDL2::SDL2-static)
else()
    set(SDL2_TARGET ${SDL2_LIBRARIES})
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

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
    src/core/ResourceManager.cpp
)

set(GAMEPLAY_SOURCES
    src/gameplay/Entity.cpp
    src/gameplay/Player.cpp
    src/gameplay/Lane.cpp
    src/gameplay/LightString.cpp
    src/gameplay/House.cpp
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
)

target_link_libraries(LightsOutChristmas PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)

if(WIN32)
    target_link_libraries(LightsOutChristmas PRIVATE SDL2::SDL2main)
endif()

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
