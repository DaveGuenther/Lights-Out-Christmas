#include "ui/GameScreen.h"
#include "core/Game.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <string>
#include <cmath>

namespace LightsOut {

static LevelConfig buildConfig(int levelIndex) {
    // Must match the configs in GameWorld.cpp exactly.
    static const LevelConfig configs[NUM_LEVELS] = {
        {"SUBURBAN STREET",   45.0f, 0.30f, 0.40f, false, false, 1, 0.05f, 0.50f, 0.65f, 0.30f},
        {"RICH NEIGHBORHOOD", 50.0f, 0.45f, 0.30f, false, false, 2, 0.15f, 0.35f, 0.40f, 0.40f},
        {"THE CUL-DE-SAC",   55.0f, 0.50f, 0.35f, true,  false, 2, 0.20f, 0.20f, 0.25f, 0.45f},
        {"CHRISTMAS EVE",    60.0f, 0.70f, 0.25f, false, true,  2, 0.30f, 0.10f, 0.10f, 0.25f},
        {"TOWN SQUARE",      65.0f, 0.80f, 0.20f, true,  true,  3, 0.40f, 0.05f, 0.05f, 0.15f},
    };
    int i = std::max(0, std::min(levelIndex, NUM_LEVELS - 1));
    return configs[i];
}

GameScreen::GameScreen(Game& game)
    : Screen(game)
    , m_world(buildConfig(game.currentLevel()), game.upgrades())
{
    m_world.onGameOver = [this]() { onGameOver(); };
    m_world.onLevelComplete = [this]() { onLevelComplete(); };
    m_world.onScorePopup = [this](int pts, Vec2 wpos) { onScorePopup(pts, wpos); };

    m_game.audio().playMusic(
        static_cast<Music>(static_cast<int>(Music::Level1) + m_game.currentLevel()),
        true);
}

void GameScreen::handleInput() {
    auto& input = m_game.input();

    if (input.isActionJustPressed(Action::Pause)) {
        m_game.pushState(GameState::Paused);
        return;
    }

    if (input.isActionJustPressed(Action::MoveUp))   m_world.playerMoveUp();
    if (input.isActionJustPressed(Action::MoveDown))  m_world.playerMoveDown();
    if (input.isActionJustPressed(Action::Bite))      m_world.playerBite();
    if (input.isActionJustPressed(Action::UsePowerUp)) m_world.playerUsePowerUp();
}

void GameScreen::update(float dt) {
    m_world.update(dt);
    m_particles.setCameraX(m_world.scrollX());
    m_particles.update(dt);

    // Adjust music intensity to darkness
    float darkness = m_world.darkness().darkness();
    m_game.audio().setMusicIntensity(darkness);

    // Combo flash
    int combo = m_world.scoreSystem().comboCount();
    if (combo > m_lastComboCount && combo > 2) {
        m_comboFlashTimer = COMBO_DISPLAY_TIME;
        m_game.audio().playSfx(SoundEffect::ComboChime);
    }
    m_lastComboCount = combo;
    if (m_comboFlashTimer > 0.0f) m_comboFlashTimer -= dt;
    if (m_darknessFlash > 0.0f) m_darknessFlash -= dt;
}

void GameScreen::render() {
    SDL_Renderer* r = m_game.renderer().sdl();
    m_world.render(r);
    m_particles.render(r);
    drawHUD(r);
}

void GameScreen::drawHUD(SDL_Renderer* r) const {
    drawScoreHUD(r);
    drawDarknessMeter(r);
    drawComboFlash(r);
    drawLaneIndicator(r);
    drawLivesHUD(r);
}

void GameScreen::drawScoreHUD(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();
    const auto& score = m_world.scoreSystem();

    // Score — show running total (prior levels + current level so far)
    int displayScore = m_game.totalScore() + score.score();
    std::string scoreStr = "SCORE " + std::to_string(displayScore);
    renderer.drawText(scoreStr, {2.0f, 2.0f}, Color::White());

    // Level name
    const char* name = buildConfig(m_game.currentLevel()).name;
    renderer.drawText(name, {RENDER_WIDTH * 0.5f, 2.0f}, {180, 180, 200}, 8, true);

    // Progress bar
    float prog = m_world.levelProgress();
    SDL_SetRenderDrawColor(r, 60, 60, 80, 255);
    SDL_FRect progBg = {RENDER_WIDTH - 52.0f, 2.0f, 50.0f, 3.0f};
    SDL_RenderFillRectF(r, &progBg);
    SDL_SetRenderDrawColor(r, 100, 200, 100, 255);
    SDL_FRect progFill = {RENDER_WIDTH - 52.0f, 2.0f, 50.0f * prog, 3.0f};
    SDL_RenderFillRectF(r, &progFill);
}

void GameScreen::drawDarknessMeter(SDL_Renderer* r) const {
    float darkness = m_world.darkness().darkness();

    // Darkness meter bar on left side
    float barH = 60.0f;
    float barY = RENDER_HEIGHT - barH - 20.0f;

    // Background
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 20, 20, 40, 180);
    SDL_FRect bg = {3.0f, barY, 5.0f, barH};
    SDL_RenderFillRectF(r, &bg);

    // Fill
    float fillH = barH * darkness;
    uint8_t red = static_cast<uint8_t>(darkness * 200);
    SDL_SetRenderDrawColor(r, red, 0, static_cast<uint8_t>(200 - red), 220);
    SDL_FRect fill = {3.0f, barY + barH - fillH, 5.0f, fillH};
    SDL_RenderFillRectF(r, &fill);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // Label
    m_game.renderer().drawText("DARK", {2.0f, barY - 6.0f}, {120, 120, 140});
}

void GameScreen::drawComboFlash(SDL_Renderer* r) const {
    if (m_comboFlashTimer <= 0.0f) return;

    auto& renderer = m_game.renderer();
    const auto& score = m_world.scoreSystem();

    float alpha = m_comboFlashTimer / COMBO_DISPLAY_TIME;
    uint8_t a = static_cast<uint8_t>(alpha * 220.0f);

    std::string comboStr = "x" + std::to_string(score.comboCount()) + " COMBO";
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Flash background
    SDL_SetRenderDrawColor(r, 255, 200, 50, static_cast<uint8_t>(a * 0.3f));
    SDL_FRect flashBg = {RENDER_WIDTH * 0.5f - 30.0f, RENDER_HEIGHT * 0.35f - 3.0f, 60.0f, 10.0f};
    SDL_RenderFillRectF(r, &flashBg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    renderer.drawText(comboStr,
                      {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.35f},
                      {255, 220, 50, a}, 8, true);
}

void GameScreen::drawLaneIndicator(SDL_Renderer* r) const {
    // Small lane indicator dots on far left
    const float dotX = 1.0f;
    for (int i = 0; i < NUM_LANES; ++i) {
        LaneType lt = laneFromIndex(i);
        float dotY = laneY(lt);
        bool active = (lt == m_world.player().currentLane());

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        if (active) {
            SDL_SetRenderDrawColor(r, 255, 255, 50, 220);
            SDL_FRect dot = {dotX, dotY - 1.0f, 2.0f, 2.0f};
            SDL_RenderFillRectF(r, &dot);
        } else {
            SDL_SetRenderDrawColor(r, 100, 100, 120, 120);
            SDL_RenderDrawPointF(r, dotX, dotY);
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}

void GameScreen::onScorePopup(int pts, Vec2 worldPos) {
    m_particles.emitScorePopup(worldPos, pts, m_world.scrollX());
    m_particles.emitSparks(worldPos, {255, 200, 50}, 4);
    m_darknessFlash = 0.2f;
}

void GameScreen::onLevelComplete() {
    m_game.audio().stopMusic();
    m_game.audio().playSfx(SoundEffect::StageComplete);
    m_game.addScore(m_world.scoreSystem().levelScore());
    m_game.nextLevel();

    if (m_game.currentLevel() >= NUM_LEVELS) {
        m_game.replaceState(GameState::GameOver);
    } else {
        m_game.replaceState(GameState::Upgrade);
    }
}

void GameScreen::drawLivesHUD(SDL_Renderer* r) const {
    // Three small squirrel-body pips in bottom-right corner
    float baseX = RENDER_WIDTH - 8.0f;
    float baseY = RENDER_HEIGHT - 10.0f;
    int   lives = m_game.lives();

    for (int i = 0; i < PLAYER_LIVES; ++i) {
        float x = baseX - static_cast<float>(i) * 10.0f;
        bool active = (i < lives);

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        // Body
        SDL_SetRenderDrawColor(r, 140, 85, 30, active ? 255 : 70);
        SDL_FRect body = {x, baseY + 2.0f, 7.0f, 6.0f};
        SDL_RenderFillRectF(r, &body);
        // Head
        SDL_FRect head = {x + 3.0f, baseY, 5.0f, 4.0f};
        SDL_RenderFillRectF(r, &head);
        // Tail
        SDL_SetRenderDrawColor(r, 180, 110, 40, active ? 255 : 70);
        SDL_FRect tail = {x - 2.0f, baseY + 1.0f, 3.0f, 3.0f};
        SDL_RenderFillRectF(r, &tail);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}

void GameScreen::onGameOver() {
    if (m_game.lives() > 0) {
        m_game.decrementLives();
        m_world.respawnPlayer();
        return;  // continue playing with remaining lives
    }
    // All lives exhausted — real game over
    m_game.resetLives();
    m_game.audio().stopMusic();
    m_game.audio().playSfx(SoundEffect::GameOver);
    m_game.addScore(m_world.scoreSystem().levelScore());
    m_game.replaceState(GameState::GameOver);
}

}  // namespace LightsOut
