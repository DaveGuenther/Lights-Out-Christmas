#pragma once
#include "core/Game.h"
#include "gameplay/GameWorld.h"
#include "rendering/Particles.h"

namespace LightsOut {

// The main gameplay screen — owns the GameWorld and draws the HUD
class GameScreen : public Screen {
public:
    explicit GameScreen(Game& game);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    GameWorld     m_world;
    ParticleSystem m_particles;

    // HUD state
    float m_comboFlashTimer  = 0.0f;
    int   m_lastComboCount   = 0;
    float m_darknessFlash    = 0.0f;  // flash when darkness meter ticks up

    // Death overlay state
    float m_deathOverlayTimer = 0.0f;  // counts down; >0 means overlay is showing
    bool  m_pendingRespawn    = false; // true=respawn when timer hits 0, false=game over

    // Power-up notification banner
    PowerUpType m_powerUpNotifType  = PowerUpType::AcornStash;
    float       m_powerUpNotifTimer = 0.0f;
    static constexpr float POWERUP_NOTIF_DURATION = 3.0f;

    void drawHUD(SDL_Renderer* r) const;
    void drawDeathOverlay(SDL_Renderer* r) const;
    void drawScoreHUD(SDL_Renderer* r) const;
    void drawDarknessMeter(SDL_Renderer* r) const;
    void drawPowerUpHUD(SDL_Renderer* r) const;
    void drawComboFlash(SDL_Renderer* r) const;
    void drawLaneIndicator(SDL_Renderer* r) const;
    void drawLivesHUD(SDL_Renderer* r) const;

    void onScorePopup(int pts, Vec2 worldPos);
    void onLevelComplete();
    void onGameOver();
};

}  // namespace LightsOut
