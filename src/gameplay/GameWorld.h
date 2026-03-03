#pragma once
#include "Player.h"
#include "House.h"
#include "BushTree.h"
#include "LightString.h"
#include "ScoreSystem.h"
#include "DarknessManager.h"
#include "Collision.h"
#include "threats/Threat.h"
#include "powerups/PowerUp.h"
#include "core/Constants.h"
#include "core/Types.h"

#include <vector>
#include <memory>
#include <random>
#include <functional>

namespace LightsOut {

// Generates and manages all world objects, handles scrolling, queries
class GameWorld {
public:
    explicit GameWorld(const LevelConfig& config, SquirrelUpgrades upgrades,
                       unsigned int randomSeed = 0);

    void update(float dt);
    void render(SDL_Renderer* renderer);

    // Input forwarding
    void playerMoveUp();
    void playerMoveDown();
    void playerBite();
    void playerUsePowerUp();

    // Respawn player after losing a life (resets player state, keeps camera)
    void respawnPlayer();

    // Queries
    float   scrollX()           const { return m_cameraX; }
    float   scrollSpeed()       const { return m_scrollSpeed; }
    Player&             player()                  { return m_player; }
    const Player&       player()            const { return m_player; }
    ScoreSystem&        scoreSystem()             { return m_score; }
    const ScoreSystem&  scoreSystem()       const { return m_score; }
    DarknessManager&    darkness()               { return m_darkness; }
    const DarknessManager& darkness()      const { return m_darkness; }
    bool    isGameOver()        const { return m_gameOver; }
    float   levelProgress()     const;   // 0→1 through the level
    float   levelLength()       const { return m_levelLength; }
    bool    isLevelComplete()   const { return m_levelComplete; }
    int     lightsRemainingInView() const;
    float   frenzyFactor()      const;   // 1.0 normally, < 1.0 in frenzy

    // Callbacks
    std::function<void()>    onGameOver;
    std::function<void()>    onLevelComplete;
    std::function<void(int, Vec2)> onScorePopup;   // (points, worldPos)

private:
    void generateChunk(float fromX);
    void spawnThreat(float worldX);
    void spawnPowerUp(float worldX);
    void pruneOffscreen();
    void checkCollisions();
    void checkPorchLights();
    void handleLightBite(LightString& ls);

    LevelConfig  m_config;
    float        m_cameraX     = 0.0f;
    float        m_scrollSpeed;
    float        m_levelLength;
    bool         m_gameOver      = false;
    bool         m_levelComplete = false;

    Player m_player;
    ScoreSystem m_score;
    DarknessManager m_darkness;

    // The active inventory power-up (collected but not yet used)
    std::unique_ptr<PowerUp> m_heldPowerUp;

    std::vector<std::shared_ptr<House>>     m_houses;
    std::vector<std::shared_ptr<BushTree>>  m_bushTrees;
    std::vector<std::shared_ptr<LightString>> m_lights;
    std::vector<std::shared_ptr<Threat>>    m_threats;
    std::vector<std::shared_ptr<PowerUp>>   m_powerups;

    // Procedural generation state
    float       m_nextHouseX    = 0.0f;
    float       m_nextThreatX   = 0.0f;
    float       m_nextPowerUpX  = 0.0f;
    float       m_genHorizon    = 0.0f;  // how far ahead we've generated
    int         m_houseCount    = 0;
    int         m_bushCount     = 0;
    std::mt19937 m_rng;

    // House blackout tracking: houseIndex → how many light strings are dark
    std::unordered_map<int, int> m_houseDarkStrands;
    std::unordered_map<int, int> m_houseTotalStrands;

    // Snow particles (rendered by GameWorld, not a separate system)
    struct SnowFlake { Vec2 pos; float speed; float drift; };
    std::vector<SnowFlake> m_snow;
    void initSnow();
    void updateSnow(float dt);
    void renderSnow(SDL_Renderer* renderer) const;
    void renderBackground(SDL_Renderer* renderer) const;
    void renderMoon(SDL_Renderer* renderer) const;
    void renderStars(SDL_Renderer* renderer) const;
    void renderLighting(SDL_Renderer* renderer) const;
};

}  // namespace LightsOut
