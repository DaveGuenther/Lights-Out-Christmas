#include "gameplay/GameWorld.h"
#include <unordered_map>
#include "gameplay/threats/Homeowner.h"
#include "gameplay/threats/Dog.h"
#include "gameplay/threats/Cat.h"
#include "gameplay/threats/Owl.h"
#include "gameplay/threats/NeighborhoodWatch.h"
#include "gameplay/powerups/AcornStash.h"
#include "gameplay/powerups/WinterCoat.h"
#include "gameplay/powerups/ShadowMode.h"
#include "gameplay/powerups/SuperChomp.h"
#include "gameplay/powerups/DecoyNut.h"
#include "gameplay/powerups/IcePatch.h"
#include "gameplay/powerups/DoubleTail.h"
#include "gameplay/powerups/FrenzyMode.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

namespace LightsOut {

// ─── Level configs ────────────────────────────────────────────────────────────
static LevelConfig getLevelConfig(int index) {
    static const LevelConfig configs[NUM_LEVELS] = {
        // 0: Suburban Street
        {"SUBURBAN STREET",  45.0f, 0.30f, 0.40f, false, false, 2, 0.10f},
        // 1: Rich Neighborhood
        {"RICH NEIGHBORHOOD", 50.0f, 0.45f, 0.30f, false, false, 4, 0.30f},
        // 2: The Cul-De-Sac
        {"THE CUL-DE-SAC",   55.0f, 0.50f, 0.35f, true,  false, 3, 0.25f},
        // 3: Christmas Eve
        {"CHRISTMAS EVE",    60.0f, 0.70f, 0.25f, false, true,  4, 0.40f},
        // 4: The Town Square
        {"TOWN SQUARE",      65.0f, 0.80f, 0.20f, true,  true,  6, 0.50f},
    };
    int i = std::max(0, std::min(index, NUM_LEVELS - 1));
    return configs[i];
}

// ─── Constructor ─────────────────────────────────────────────────────────────
GameWorld::GameWorld(const LevelConfig& config, SquirrelUpgrades upgrades,
                     unsigned int randomSeed)
    : m_config(config)
    , m_scrollSpeed(config.scrollSpeed)
    , m_levelLength(3000.0f)
    , m_rng(randomSeed == 0 ? static_cast<unsigned>(SDL_GetTicks()) : randomSeed)
{
    m_player.setUpgrades(upgrades);

    // Score callbacks
    m_score.onScore = [this](const ScoreEvent& evt) {
        (void)evt;
    };

    // Darkness callbacks
    m_darkness.onNeighborhoodDark = [this]() {
        if (onScorePopup) onScorePopup(NEIGHBORHOOD_BONUS, m_player.position);
    };

    // Player death
    m_player.onDeath = [this]() {
        m_gameOver = true;
        if (onGameOver) onGameOver();
    };

    initSnow();
    generateChunk(0.0f);
    generateChunk(RENDER_WIDTH * 0.6f);
}

// ─── Update ──────────────────────────────────────────────────────────────────
void GameWorld::update(float dt) {
    if (m_gameOver || m_levelComplete) return;

    float frenzy = frenzyFactor();

    // Scroll world
    m_cameraX += m_scrollSpeed * dt * frenzy;

    // Generate ahead
    while (m_genHorizon < m_cameraX + RENDER_WIDTH * 2.5f) {
        generateChunk(m_genHorizon);
    }

    // Update entities
    for (auto& h : m_houses) h->update(dt);
    for (auto& t : m_threats) {
        if (auto* owl = dynamic_cast<Owl*>(t.get())) {
            bool onBranch = (m_player.currentLane() == LaneType::Branch &&
                             std::abs(m_player.position.x - owl->position.x) < 40.0f);
            owl->playerOnBranch(onBranch);
        }
        t->update(dt);
        // Update threat target toward player world position
        if (t->isAlerted())
            t->alert({PLAYER_START_X + m_cameraX, laneY(m_player.currentLane())});
    }
    m_player.update(dt);
    for (auto& p : m_powerups) p->update(dt);

    checkCollisions();
    checkPorchLights();
    pruneOffscreen();
    updateSnow(dt);

    // Audio intensity from darkness meter
    m_score.update(dt);

    // Level complete?
    if (m_cameraX >= m_levelLength && !m_levelComplete) {
        m_levelComplete = true;
        if (onLevelComplete) onLevelComplete();
    }
}

// ─── Render ──────────────────────────────────────────────────────────────────
void GameWorld::render(SDL_Renderer* renderer) {
    renderBackground(renderer);
    renderMoon(renderer);
    renderStars(renderer);

    for (auto& h : m_houses)  h->render(renderer, m_cameraX);
    for (auto& t : m_threats) t->render(renderer, m_cameraX);
    for (auto& p : m_powerups) p->render(renderer, m_cameraX);
    m_player.render(renderer, m_cameraX);

    renderSnow(renderer);

    // Draw lane guide lines (subtle)
    static const LaneType lanes[] = {LaneType::Rooftop, LaneType::PowerLine,
                                      LaneType::Branch,  LaneType::Fence};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 100, 100, 140, 30);
    for (auto lane : lanes) {
        float y = laneY(lane);
        SDL_RenderDrawLineF(renderer, 0, y, static_cast<float>(RENDER_WIDTH), y);
    }
    // Ground line
    SDL_SetRenderDrawColor(renderer, 60, 80, 40, 80);
    SDL_RenderDrawLineF(renderer, 0, LANE_GROUND_Y,
                        static_cast<float>(RENDER_WIDTH), LANE_GROUND_Y);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// ─── Input ───────────────────────────────────────────────────────────────────
void GameWorld::playerMoveUp()   { if (!m_gameOver) m_player.moveUp(); }
void GameWorld::playerMoveDown() { if (!m_gameOver) m_player.moveDown(); }
void GameWorld::playerBite()     { if (!m_gameOver) {
    m_player.tryBite(m_lights);
    // Clean up fully-dark strings
    m_lights.erase(
        std::remove_if(m_lights.begin(), m_lights.end(),
                       [](const std::shared_ptr<LightString>& ls) {
                           return ls->isFullyOff();
                       }),
        m_lights.end());
}}

void GameWorld::playerUsePowerUp() {
    if (!m_heldPowerUp || m_gameOver) return;

    PowerUpType type = m_heldPowerUp->powerUpType();
    m_heldPowerUp->apply(m_player);

    // Side effects for world-level power-ups
    if (type == PowerUpType::IcePatch) {
        // Freeze closest threat
        float best = 9999.0f;
        Threat* target = nullptr;
        for (auto& t : m_threats) {
            float dx = t->position.x - (PLAYER_START_X + m_cameraX);
            float dy = t->position.y - m_player.position.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < best) { best = dist; target = t.get(); }
        }
        if (target) target->freeze(POWERUP_DURATION_ICE);
    } else if (type == PowerUpType::DecoyNut) {
        // Distract all nearby threats toward a position ahead of player
        Vec2 decoyPos = {PLAYER_START_X + m_cameraX + 40.0f, LANE_GROUND_Y};
        for (auto& t : m_threats) {
            float dx = t->position.x - (PLAYER_START_X + m_cameraX);
            if (std::abs(dx) < 80.0f) t->distract(decoyPos);
        }
    }
    m_heldPowerUp.reset();
}

// ─── Generation ──────────────────────────────────────────────────────────────
void GameWorld::generateChunk(float fromX) {
    std::uniform_real_distribution<float> widthDist(HOUSE_MIN_WIDTH, HOUSE_MAX_WIDTH);
    std::uniform_real_distribution<float> gapDist(HOUSE_MIN_GAP, HOUSE_MAX_GAP);
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    float houseW = widthDist(m_rng);
    HouseStyle style = HouseStyle::Simple;
    if (m_houseCount % 5 == 4) style = HouseStyle::Elaborate;

    int houseIdx = m_houseCount++;
    auto house = std::make_shared<House>(
        fromX, houseW, style, houseIdx,
        m_config.lightStringsPerHouse,
        m_config.tangledLightProbability);

    m_houseTotalStrands[houseIdx] = static_cast<int>(house->lightStrings().size());
    m_houseDarkStrands[houseIdx]  = 0;

    // Connect strand callbacks
    for (auto& ls : house->lightStrings()) {
        ls->onDark = [this, houseIdx](int bulbCount, bool chain) {
            m_score.onLightOff(bulbCount);
            m_darkness.onLightOff(bulbCount);

            m_houseDarkStrands[houseIdx]++;
            if (m_houseDarkStrands[houseIdx] >= m_houseTotalStrands[houseIdx]) {
                m_score.onHouseBlackout(houseIdx);
                m_darkness.onHouseBlackout();
            }
            if (chain) m_score.onChainReaction(1);
        };
        m_lights.push_back(ls);
    }

    m_houses.push_back(house);

    // Maybe spawn a threat near this house
    if (prob(m_rng) < m_config.threatDensity) {
        spawnThreat(fromX + houseW * 0.5f);
    }

    // Maybe spawn a power-up on a lane
    if (prob(m_rng) < m_config.powerUpFrequency) {
        spawnPowerUp(fromX + houseW * 0.3f);
    }

    m_genHorizon = fromX + houseW + gapDist(m_rng);
}

void GameWorld::spawnThreat(float worldX) {
    std::uniform_int_distribution<int> typeDist(0, 4);
    int choice = typeDist(m_rng);

    if (m_config.hasNeighborhoodWatch && choice == 4) {
        auto t = std::make_shared<NeighborhoodWatch>(worldX + 60.0f);
        m_threats.push_back(t);
        return;
    }

    switch (choice % 4) {
    case 0: {
        // Grumpy or dad homeowner
        std::uniform_int_distribution<int> pDist(0,1);
        auto pers = pDist(m_rng) == 0
                    ? Homeowner::Personality::GrumpyOldMan
                    : Homeowner::Personality::DadInPajamas;
        auto t = std::make_shared<Homeowner>(worldX, LANE_GROUND_Y - 16.0f, pers);
        m_threats.push_back(t);
        break;
    }
    case 1: {
        std::uniform_real_distribution<float> chainDist(20.0f, DOG_CHAIN_RADIUS);
        auto t = std::make_shared<Dog>(worldX, LANE_GROUND_Y - 8.0f, chainDist(m_rng));
        m_threats.push_back(t);
        break;
    }
    case 2: {
        auto t = std::make_shared<Cat>(worldX, LANE_ROOFTOP_Y - 7.0f);
        m_threats.push_back(t);
        break;
    }
    case 3: {
        auto t = std::make_shared<Owl>(worldX, LANE_BRANCH_Y - 14.0f);
        m_threats.push_back(t);
        break;
    }
    }
}

void GameWorld::spawnPowerUp(float worldX) {
    static const PowerUpType types[] = {
        PowerUpType::AcornStash, PowerUpType::WinterCoat, PowerUpType::ShadowMode,
        PowerUpType::SuperChomp, PowerUpType::DecoyNut,   PowerUpType::IcePatch,
        PowerUpType::DoubleTail, PowerUpType::FrenzyMode
    };
    std::uniform_int_distribution<int> typeDist(0, 7);
    std::uniform_int_distribution<int> laneDist(0, NUM_LANES - 1);
    PowerUpType pt = types[typeDist(m_rng)];
    float wy = laneY(laneFromIndex(laneDist(m_rng))) - 8.0f;

    std::shared_ptr<PowerUp> pu;
    switch (pt) {
    case PowerUpType::AcornStash:  pu = std::make_shared<AcornStash>(worldX, wy); break;
    case PowerUpType::WinterCoat:  pu = std::make_shared<WinterCoat>(worldX, wy); break;
    case PowerUpType::ShadowMode:  pu = std::make_shared<ShadowMode>(worldX, wy); break;
    case PowerUpType::SuperChomp:  pu = std::make_shared<SuperChomp>(worldX, wy); break;
    case PowerUpType::DecoyNut:    pu = std::make_shared<DecoyNut>(worldX, wy); break;
    case PowerUpType::IcePatch:    pu = std::make_shared<IcePatch>(worldX, wy); break;
    case PowerUpType::DoubleTail:  pu = std::make_shared<DoubleTail>(worldX, wy); break;
    case PowerUpType::FrenzyMode:  pu = std::make_shared<FrenzyMode>(worldX, wy); break;
    }
    if (pu) {
        pu->onCollect = [this](PowerUpType type) {
            // First power-up is auto-applied; subsequent go to inventory
            if (!m_heldPowerUp) {
                // Apply immediately for direct-apply types
                if (type == PowerUpType::AcornStash ||
                    type == PowerUpType::WinterCoat ||
                    type == PowerUpType::ShadowMode ||
                    type == PowerUpType::SuperChomp ||
                    type == PowerUpType::DoubleTail ||
                    type == PowerUpType::FrenzyMode) {
                    // Already applied in apply(); nothing extra needed
                }
            }
            (void)type;
        };
        m_powerups.push_back(pu);
    }
}

// ─── Collision ───────────────────────────────────────────────────────────────
void GameWorld::checkCollisions() {
    Rect playerBounds = m_player.bounds();
    // Offset player bounds to world space
    playerBounds.x += m_cameraX - PLAYER_START_X;

    // Power-up collection
    for (auto& pu : m_powerups) {
        if (!pu->alive || pu->collected()) continue;
        Rect puBounds = pu->bounds();
        if (Collision::overlaps(playerBounds, puBounds)) {
            pu->apply(m_player);
        }
    }

    // Threat contact (unless invincible / shadow)
    if (!m_player.isInvincible() && !m_player.isShadowMode()) {
        for (auto& t : m_threats) {
            if (!t->alive) continue;

            // Neighborhood Watch — special ground check
            if (auto* nw = dynamic_cast<NeighborhoodWatch*>(t.get())) {
                Rect screenBounds = m_player.bounds();
                if (m_player.currentLane() == LaneType::Ground &&
                    nw->detectsGroundPlayer(screenBounds)) {
                    m_gameOver = true;
                    if (onGameOver) onGameOver();
                    return;
                }
                continue;
            }

            if (t->caughtPlayer(playerBounds)) {
                m_gameOver = true;
                if (onGameOver) onGameOver();
                return;
            }
        }
    }
}

void GameWorld::checkPorchLights() {
    if (m_player.isShadowMode()) return;

    Vec2 playerWorld = {PLAYER_START_X + m_cameraX,
                        m_player.position.y + m_player.height * 0.5f};

    for (auto& house : m_houses) {
        if (!house->porchLightOn()) continue;
        Vec2 lp = house->porchLightPos();
        float dx = playerWorld.x - lp.x;
        float dy = playerWorld.y - lp.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < PORCH_LIGHT_RADIUS) {
            // Alert nearest homeowner associated with this house
            for (auto& t : m_threats) {
                if (auto* hw = dynamic_cast<Homeowner*>(t.get())) {
                    float tdx = t->position.x - house->position.x;
                    if (std::abs(tdx) < house->width * 2.0f) {
                        hw->alert(playerWorld);
                    }
                }
            }
            house->resetPorchLight(5.0f);
        }
    }
}

void GameWorld::pruneOffscreen() {
    auto prune = [&](auto& vec) {
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [this](const auto& e) {
                    return !e->alive || e->isOffscreen(m_cameraX,
                        static_cast<float>(RENDER_WIDTH));
                }),
            vec.end());
    };
    prune(m_houses);
    prune(m_lights);
    prune(m_threats);
    prune(m_powerups);
}

// ─── Snow ────────────────────────────────────────────────────────────────────
void GameWorld::initSnow() {
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(RENDER_WIDTH));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(RENDER_HEIGHT));
    std::uniform_real_distribution<float> sDist(SNOW_FALL_SPEED * 0.7f, SNOW_FALL_SPEED * 1.3f);
    std::uniform_real_distribution<float> dDist(-0.5f, 0.5f);

    m_snow.resize(SNOW_PARTICLES);
    for (auto& f : m_snow) {
        f.pos   = {xDist(m_rng), yDist(m_rng)};
        f.speed = sDist(m_rng);
        f.drift = dDist(m_rng);
    }
}

void GameWorld::updateSnow(float dt) {
    for (auto& f : m_snow) {
        f.pos.y += f.speed * dt;
        f.pos.x += f.drift;
        if (f.pos.y > RENDER_HEIGHT) { f.pos.y = -2.0f; }
        if (f.pos.x < 0) f.pos.x += RENDER_WIDTH;
        if (f.pos.x > RENDER_WIDTH) f.pos.x -= RENDER_WIDTH;
    }
}

void GameWorld::renderSnow(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 220, 235, 255, 180);
    for (const auto& f : m_snow) {
        SDL_FRect flake = {f.pos.x, f.pos.y, 1.0f, 2.0f};
        SDL_RenderFillRectF(renderer, &flake);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void GameWorld::renderBackground(SDL_Renderer* renderer) const {
    // Sky gradient: top = very dark blue, bottom = slightly lighter
    for (int y = 0; y < RENDER_HEIGHT; y += 4) {
        float t = static_cast<float>(y) / RENDER_HEIGHT;
        uint8_t r = static_cast<uint8_t>(8  + static_cast<int>(t * 10));
        uint8_t g = static_cast<uint8_t>(12 + static_cast<int>(t * 8));
        uint8_t b = static_cast<uint8_t>(35 + static_cast<int>(t * 15));
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer, 0, y, RENDER_WIDTH, y);
        SDL_RenderDrawLine(renderer, 0, y+1, RENDER_WIDTH, y+1);
        SDL_RenderDrawLine(renderer, 0, y+2, RENDER_WIDTH, y+2);
        SDL_RenderDrawLine(renderer, 0, y+3, RENDER_WIDTH, y+3);
    }

    // Ground strip
    SDL_SetRenderDrawColor(renderer, 30, 45, 25, 255);
    SDL_Rect ground = {0, static_cast<int>(LANE_GROUND_Y),
                       RENDER_WIDTH, RENDER_HEIGHT - static_cast<int>(LANE_GROUND_Y)};
    SDL_RenderFillRect(renderer, &ground);

    // Snow on ground
    SDL_SetRenderDrawColor(renderer, 200, 220, 240, 255);
    SDL_Rect snowGround = {0, static_cast<int>(LANE_GROUND_Y),
                           RENDER_WIDTH, 4};
    SDL_RenderFillRect(renderer, &snowGround);
}

void GameWorld::renderMoon(SDL_Renderer* renderer) const {
    // Moon in top-right
    int mx = RENDER_WIDTH - 30, my = 18;
    int r = 10;
    SDL_SetRenderDrawColor(renderer, 240, 240, 220, 255);
    for (int dy = -r; dy <= r; ++dy) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(r*r - dy*dy)));
        SDL_RenderDrawLine(renderer, mx - dx, my + dy, mx + dx, my + dy);
    }
    // Crescent shadow
    SDL_SetRenderDrawColor(renderer, 10, 15, 40, 255);
    for (int dy = -r; dy <= r; ++dy) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(r*r - dy*dy)));
        SDL_RenderDrawLine(renderer, mx - dx + 4, my + dy, mx + dx, my + dy);
    }
}

void GameWorld::renderStars(SDL_Renderer* renderer) const {
    // Fixed star positions based on a deterministic seed
    std::mt19937 starRng(0xDEADBEEF);
    std::uniform_int_distribution<int> xDist(0, RENDER_WIDTH);
    std::uniform_int_distribution<int> yDist(0, static_cast<int>(RENDER_HEIGHT * 0.55f));
    std::uniform_int_distribution<int> brightDist(120, 240);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 50; ++i) {
        int sx = xDist(starRng);
        int sy = yDist(starRng);
        uint8_t bright = static_cast<uint8_t>(brightDist(starRng));
        SDL_SetRenderDrawColor(renderer, bright, bright, bright, bright);
        SDL_RenderDrawPoint(renderer, sx, sy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// ─── Queries ─────────────────────────────────────────────────────────────────
float GameWorld::levelProgress() const {
    return std::max(0.0f, std::min(1.0f, m_cameraX / m_levelLength));
}

float GameWorld::frenzyFactor() const {
    return m_player.frenzySlowFactor();
}

int GameWorld::lightsRemainingInView() const {
    int count = 0;
    for (const auto& ls : m_lights) {
        float sx = ls->position.x - m_cameraX;
        if (sx > -20.0f && sx < RENDER_WIDTH + 20.0f && !ls->isFullyOff())
            ++count;
    }
    return count;
}

}  // namespace LightsOut
