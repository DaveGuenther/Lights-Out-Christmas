#pragma once
#include "core/Game.h"
#include "gameplay/Player.h"
#include "gameplay/LightString.h"
#include "gameplay/threats/Threat.h"
#include "rendering/Particles.h"
#include "core/PlatformData.h"
#include <vector>
#include <memory>
#include <random>

namespace LightsOut {

// Final boss screen — the town square Christmas tree.
// Camera scrolls in, locks on the tree, then the player must bite off all the
// lights while owls, cats and dogs guard the tree.
class TownSquareBossScreen : public Screen {
public:
    explicit TownSquareBossScreen(Game& game);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    // ── Scene constants ──────────────────────────────────────────────────────
    static constexpr float TREE_WORLD_X  = 490.0f;   // left edge of tree_large in world space
    static constexpr float LOCK_CAM_X    = 256.0f;   // camera stops scrolling here
    static constexpr float SCROLL_SPEED  = 45.0f;    // px/sec until lock

    // Runtime tree dimensions (probed from sprite at load time)
    float m_treeSpriteY = 0.0f;
    float m_treeSpriteW = 177.0f;
    float m_treeSpriteH = 275.0f;
    std::vector<Platform> m_treePlatforms;

    // ── Entities ─────────────────────────────────────────────────────────────
    Player         m_player;
    ParticleSystem m_particles;
    std::vector<std::shared_ptr<LightString>> m_lights;
    std::vector<std::shared_ptr<Threat>>      m_threats;

    // ── State ────────────────────────────────────────────────────────────────
    float m_cameraX      = 0.0f;
    bool  m_camLocked    = false;
    int   m_totalLights  = 0;
    bool  m_won          = false;
    float m_winTimer     = 0.0f;

    // Death overlay (mirrors GameScreen)
    float m_deathTimer     = 0.0f;
    bool  m_pendingRespawn = false;

    // ── Snow ─────────────────────────────────────────────────────────────────
    struct Snowflake { Vec2 pos; float speed; float drift; };
    std::vector<Snowflake> m_snow;
    std::mt19937 m_rng;

    // ── Init ─────────────────────────────────────────────────────────────────
    void buildTree();
    void spawnThreats();
    void initSnow();

    // ── Per-frame helpers ─────────────────────────────────────────────────────
    void updateSnow(float dt);
    void checkCollisions();
    void resolvePlatformCollision();
    int  countLightsRemaining() const;
    void doRespawn();
    void onPlayerDeath();

    // ── Rendering ────────────────────────────────────────────────────────────
    void renderBackground(SDL_Renderer* r) const;
    void renderTree(SDL_Renderer* r) const;
    void renderSnow(SDL_Renderer* r) const;
    void renderLaneGuides(SDL_Renderer* r) const;
    void drawHUD(SDL_Renderer* r) const;
    void drawDeathOverlay(SDL_Renderer* r) const;
    void drawWinTransition(SDL_Renderer* r) const;
};

}  // namespace LightsOut
