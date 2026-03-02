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

    void drawHUD(SDL_Renderer* r) const;
    void drawScoreHUD(SDL_Renderer* r) const;
    void drawDarknessMeter(SDL_Renderer* r) const;
    void drawPowerUpHUD(SDL_Renderer* r) const;
    void drawComboFlash(SDL_Renderer* r) const;
    void drawLaneIndicator(SDL_Renderer* r) const;

    void onScorePopup(int pts, Vec2 worldPos);
    void onLevelComplete();
    void onGameOver();
};

}  // namespace LightsOut
