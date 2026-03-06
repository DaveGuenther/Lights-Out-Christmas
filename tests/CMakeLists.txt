# Try system Catch2 first; if not found, download it automatically
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found via find_package — fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

# Sources shared between tests and game (no SDL dependency)
set(TESTABLE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/gameplay/ScoreSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/Collision.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/Lane.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/LightString.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/Entity.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/DarknessManager.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/House.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/BushTree.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/PowerUp.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/AcornStash.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/WinterCoat.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/ShadowMode.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/SuperChomp.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/DecoyNut.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/IcePatch.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/DoubleTail.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/powerups/FrenzyMode.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/threats/Threat.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/threats/Homeowner.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/threats/Dog.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/threats/Cat.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/threats/Owl.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/threats/NeighborhoodWatch.cpp
    ${CMAKE_SOURCE_DIR}/src/gameplay/Player.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SpriteRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ResourceManager.cpp
)

set(TEST_SOURCES
    test_main.cpp
    test_score_system.cpp
    test_collision.cpp
    test_lane_system.cpp
    test_light_string.cpp
    test_power_ups.cpp
    test_threats.cpp
    test_darkness_manager.cpp
    test_house.cpp
    test_bush_tree.cpp
)

add_executable(LightsOutTests ${TEST_SOURCES} ${TESTABLE_SOURCES})

target_include_directories(LightsOutTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(LightsOutTests PRIVATE
    Catch2::Catch2WithMain
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
)
# SDL2main is not linked here — it hijacks WinMain, which conflicts with
# Catch2WithMain's own entry point.  The game executable links it separately.

include(Catch)
catch_discover_tests(LightsOutTests)
