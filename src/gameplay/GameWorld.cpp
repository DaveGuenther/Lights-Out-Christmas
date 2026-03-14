#include "gameplay/GameWorld.h"
#include "core/SpriteRegistry.h"
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
    // name, scrollSpeed, threatDensity, powerUpFrequency,
    // homeownersCoordinate, hasNeighborhoodWatch,
    // lightStringsPerHouse (strands per tier on hard houses),
    // tangledLightProbability, darkHouseProbability,
    // easyHouseFraction, mediumHouseFraction
    // (hard fraction = 1 - easy - medium, for lit houses only)
    static const LevelConfig configs[NUM_LEVELS] = {
        // 0: Suburban Street — easy: mostly 1-tier, a few 2-tier, rarely 3-tier
        {"SUBURBAN STREET",   45.0f, 0.15f, 0.40f, false, false, 1, 0.05f, 0.50f, 0.65f, 0.30f},
        // 1: Rich Neighborhood — moderate mix
        {"RICH NEIGHBORHOOD", 50.0f, 0.22f, 0.30f, false, false, 2, 0.15f, 0.35f, 0.40f, 0.40f},
        // 2: The Cul-De-Sac — medium-hard
        {"THE CUL-DE-SAC",   55.0f, 0.25f, 0.35f, true,  false, 2, 0.20f, 0.20f, 0.25f, 0.45f},
        // 3: Christmas Eve — hard: mostly 3-tier
        {"CHRISTMAS EVE",    60.0f, 0.35f, 0.25f, false, true,  2, 0.30f, 0.10f, 0.10f, 0.25f},
        // 4: The Town Square — hardest: almost all 3-tier
        {"TOWN SQUARE",      65.0f, 0.40f, 0.20f, true,  true,  3, 0.40f, 0.05f, 0.05f, 0.15f},
    };
    int i = std::max(0, std::min(index, NUM_LEVELS - 1));
    return configs[i];
}

// ─── Constructor ─────────────────────────────────────────────────────────────
GameWorld::GameWorld(const LevelConfig& config, SquirrelUpgrades upgrades,
                     unsigned int randomSeed)
    : m_config(config)
    , m_scrollSpeed(config.scrollSpeed)
    , m_levelLength(config.scrollSpeed * 90.0f)  // ~90-second level
    , m_rng(randomSeed == 0 ? static_cast<unsigned>(SDL_GetTicks()) : randomSeed)
{
    m_player.setUpgrades(upgrades);

    // Load house asset manifest
    char* sdlBase = SDL_GetBasePath();
    if (sdlBase) {
        std::string assetsDir = std::string(sdlBase) + "assets";
        m_houseAssets.load(assetsDir);
        SDL_free(sdlBase);
    }

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
    generateChunk(m_genHorizon);
}

// ─── Update ──────────────────────────────────────────────────────────────────
void GameWorld::update(float dt) {
    if (m_gameOver || m_levelComplete) return;

    float frenzy = frenzyFactor();

    // Scroll world
    m_cameraX += m_scrollSpeed * dt * frenzy;

    // Player screen position:
    //   - With input:  moveHorizontal already updated screenX; sync world position from it.
    //   - No input:    squirrel is world-fixed → screenX drifts left with the scroll.
    //                  Clamp at PLAYER_SCREEN_X_MIN so it never leaves the screen.
    bool didMoveHoriz    = m_playerMovedHoriz;
    m_playerMovedHoriz   = false;

    if (didMoveHoriz) {
        m_player.position.x = m_cameraX + m_player.screenX();
    } else {
        float newSX = m_player.screenX() - m_scrollSpeed * dt * frenzy;
        newSX = std::max(PLAYER_SCREEN_X_MIN, std::min(PLAYER_SCREEN_X_MAX, newSX));
        m_player.setScreenX(newSX);
        m_player.position.x = m_cameraX + newSX;
    }

    // Drive idle/run animation — running when actively moving or pinned at left wall
    bool atLeftWall = (m_player.screenX() <= PLAYER_SCREEN_X_MIN + 0.5f);
    m_player.setMoving(didMoveHoriz || atLeftWall);

    // Generate ahead — keep at least 3 screen-widths of houses in the buffer
    while (m_genHorizon < m_cameraX + RENDER_WIDTH * 3.0f) {
        generateChunk(m_genHorizon);
    }

    // Update entities
    for (auto& h : m_houses)     h->update(dt);
    for (auto& bt : m_bushTrees) bt->update(dt);
    for (auto& te : m_trees)     te->update(dt);

    const float playerWorldX = m_player.position.x;
    const Vec2  playerWorld  = {playerWorldX, laneY(m_player.currentLane())};

    for (auto& t : m_threats) {
        // ── Owl: patience/attack based on player being on the fence lane nearby ──
        if (auto* owl = dynamic_cast<Owl*>(t.get())) {
            bool onFence = (m_player.currentLane() == LaneType::Fence &&
                            std::abs(playerWorldX - owl->position.x) < 60.0f);
            owl->playerOnFence(onFence);
        }

        // ── Proximity alerting for all other threats ──────────────────────────
        // Alert any un-alerted threat that has scrolled to within one screen
        // width of the player (i.e., it is visible or about to be visible).
        if (!t->isAlerted()) {
            float screenX = t->position.x - m_cameraX;
            if (screenX > -16.0f && screenX < static_cast<float>(RENDER_WIDTH) + 16.0f) {
                t->alert(playerWorld);
            }
        }

        t->update(dt);

        // Keep alerted threats' target locked on the player's current position
        if (t->isAlerted())
            t->alert(playerWorld);
    }
    m_player.update(dt);
    m_player.tickDropTimer(dt);
    resolvePlatformCollision();
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

    for (auto& h : m_houses)     h->render(renderer, m_cameraX);
    for (auto& bt : m_bushTrees) bt->render(renderer, m_cameraX);
    for (auto& te : m_trees)     te->render(renderer, m_cameraX);
    for (auto& t : m_threats)    t->render(renderer, m_cameraX);
    for (auto& p : m_powerups) p->render(renderer, m_cameraX);
    m_player.render(renderer, m_cameraX);

    renderLighting(renderer);
    renderSnow(renderer);

    // Draw lane guide lines (subtle)
    static const LaneType lanes[] = {LaneType::Rooftop, LaneType::Fence};
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
void GameWorld::playerJump()  { if (!m_gameOver) m_player.jump(); }
void GameWorld::playerDrop()  { if (!m_gameOver) m_player.drop(); }
void GameWorld::playerBite()     { if (!m_gameOver) {
    m_player.tryBite(m_lights, m_cameraX);
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
            float dx = t->position.x - m_player.position.x;
            float dy = t->position.y - m_player.position.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < best) { best = dist; target = t.get(); }
        }
        if (target) target->freeze(POWERUP_DURATION_ICE);
    } else if (type == PowerUpType::DecoyNut) {
        // Distract all nearby threats toward a position ahead of player
        Vec2 decoyPos = {m_player.position.x + 40.0f, LANE_GROUND_Y};
        for (auto& t : m_threats) {
            float dx = t->position.x - m_player.position.x;
            if (std::abs(dx) < 80.0f) t->distract(decoyPos);
        }
    }
    m_heldPowerUp.reset();
}

void GameWorld::playerMoveHorizontal(float dir, float dt) {
    m_playerMovedHoriz = (dir != 0.0f);
    if (!m_gameOver && m_playerMovedHoriz) m_player.moveHorizontal(dir, dt);
}

void GameWorld::respawnPlayer() {
    m_gameOver = false;
    m_player.respawn();
    m_player.position.x = m_cameraX + m_player.screenX();

    // Freeze ALL threats so the player has breathing room after respawn
    for (auto& t : m_threats)
        t->freeze(RESPAWN_THREAT_FREEZE);
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
// Wire onDark callback for a light string and add to the world light list
static void wireLightString(GameWorld* world,
                             std::vector<std::shared_ptr<LightString>>& lights,
                             const std::shared_ptr<LightString>& ls,
                             ScoreSystem& score, DarknessManager& darkness,
                             int houseIdx,
                             std::unordered_map<int,int>& darkStrands,
                             std::unordered_map<int,int>& totalStrands)
{
    ls->onDark = [&score, &darkness, houseIdx, &darkStrands, &totalStrands](int bulbCount, bool chain) {
        score.onLightOff(bulbCount);
        darkness.onLightOff(bulbCount);
        darkStrands[houseIdx]++;
        if (darkStrands[houseIdx] >= totalStrands[houseIdx]) {
            score.onHouseBlackout(houseIdx);
            darkness.onHouseBlackout();
        }
        if (chain) score.onChainReaction(1);
    };
    lights.push_back(ls);
    (void)world;
}

// ─── Generation ──────────────────────────────────────────────────────────────
void GameWorld::generateChunk(float fromX) {
    std::uniform_real_distribution<float> gapDist(HOUSE_MIN_GAP, HOUSE_MAX_GAP);
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    // ── Choose house size and asset ──────────────────────────────────────────
    // House size is random weighted toward smaller on early levels
    HouseSize houseSize;
    {
        float r = prob(m_rng);
        // easyHouseFraction≈small, medium≈medium, remainder≈large
        if (r < m_config.easyHouseFraction)
            houseSize = HouseSize::Small;
        else if (r < m_config.easyHouseFraction + m_config.mediumHouseFraction)
            houseSize = HouseSize::Medium;
        else
            houseSize = HouseSize::Large;
    }

    const HouseAsset* asset = m_houseAssets.select(houseSize, m_rng);
    // Fallback to a minimal inline asset if none loaded (graceful degradation)
    static HouseAsset s_fallback{"fallback", "", "", "", 256.0f};
    if (!asset) asset = &s_fallback;

    float houseW = asset->pixelWidth;

    HouseStyle style = HouseStyle::Simple;
    if (m_houseCount % 5 == 4) style = HouseStyle::Elaborate;

    // ── Determine which tiers are lit ────────────────────────────────────────
    bool hasTop = false, hasMid = false, hasGnd = false;
    int  strands = std::max(1, m_config.lightStringsPerHouse);

    if (prob(m_rng) >= m_config.darkHouseProbability) {
        float r = prob(m_rng);
        HouseDifficulty diff;
        if      (r < m_config.easyHouseFraction)
            diff = HouseDifficulty::Easy;
        else if (r < m_config.easyHouseFraction + m_config.mediumHouseFraction)
            diff = HouseDifficulty::Medium;
        else
            diff = HouseDifficulty::Hard;

        if (diff == HouseDifficulty::Easy) {
            std::uniform_int_distribution<int> tierDist(0, 2);
            switch (tierDist(m_rng)) {
            case 0: hasTop = true; break;
            case 1: hasMid = true; break;
            case 2: hasGnd = true; break;
            }
        } else if (diff == HouseDifficulty::Medium) {
            if (prob(m_rng) < 0.5f) { hasTop = true; hasMid = true; }
            else                    { hasMid = true; hasGnd = true; }
        } else {
            hasTop = true; hasMid = true; hasGnd = true;
        }
    }

    int houseIdx = m_houseCount++;
    auto house = std::make_shared<House>(
        fromX, style, houseIdx, *asset,
        hasTop, hasMid, hasGnd,
        strands, m_config.tangledLightProbability,
        nullptr, &m_houseAssets.bushAssets());

    m_houseTotalStrands[houseIdx] = static_cast<int>(house->lightStrings().size());
    m_houseDarkStrands[houseIdx]  = 0;

    // Wire house light strand callbacks
    for (auto& ls : house->lightStrings()) {
        wireLightString(this, m_lights, ls, m_score, m_darkness,
                        houseIdx, m_houseDarkStrands, m_houseTotalStrands);
    }

    // Bring house-owned BushTrees into the world's bush list
    // (they are already rendered by the house, so no need to add to m_bushTrees)
    // Any lit bushes from gap get added below.

    m_houses.push_back(house);

    // Maybe spawn a threat near this house
    if (prob(m_rng) < m_config.threatDensity) {
        spawnThreat(fromX + houseW * 0.5f);
    }

    // Maybe spawn a power-up on a lane
    if (prob(m_rng) < m_config.powerUpFrequency) {
        spawnPowerUp(fromX + houseW * 0.3f);
    }

    float gap = gapDist(m_rng);
    m_genHorizon = fromX + houseW + gap;

    // Fill gap with trees (replaces old standalone-bush logic)
    generateTreeGap(fromX + houseW, gap);
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
        auto t = std::make_shared<Owl>(worldX, LANE_FENCE_Y - 14.0f);
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
    // position.x is already world space (= cameraX + screenX), so bounds() needs no offset
    Rect playerBounds = m_player.bounds();

    // Power-up collection
    for (auto& pu : m_powerups) {
        if (!pu->alive || pu->collected()) continue;
        Rect puBounds = pu->bounds();
        if (Collision::overlaps(playerBounds, puBounds)) {
            PowerUpType t = pu->powerUpType();
            pu->apply(m_player);
            if (onPowerUpCollect) onPowerUpCollect(t);
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

    Vec2 playerWorld = {m_player.position.x,
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
    // Use the same 3-screen-width lookahead as generation so we never prune
    // entities that were just generated ahead of the camera.
    const float lookahead = static_cast<float>(RENDER_WIDTH) * 3.0f;
    auto prune = [&](auto& vec) {
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [this, lookahead](const auto& e) {
                    return !e->alive || e->isOffscreen(m_cameraX, lookahead);
                }),
            vec.end());
    };
    prune(m_houses);
    prune(m_bushTrees);
    prune(m_trees);
    prune(m_lights);
    prune(m_threats);
    prune(m_powerups);
}

// ─── Platform collision ───────────────────────────────────────────────────────
void GameWorld::resolvePlatformCollision() {
    Player& p = m_player;
    if (p.state() == PlayerState::Dead) return;

    const float prevFeet = p.prevFeetY();  // feet Y before this frame's movement
    const float newFeet  = p.position.y + p.height;
    const float pLeft    = p.position.x;
    const float pRight   = p.position.x + p.width;

    // Only resolve when falling (velocityY >= 0); ascending passes through all platforms
    bool falling = (p.velocityY() >= 0.0f);

    // Mark as not grounded; platform/floor code below will re-ground if applicable
    bool landedThisFrame = false;
    float bestPlatformY  = 1e9f;  // lowest surface we can land on this frame
    LaneType bestTier    = LaneType::Ground;

    // Slope support: when grounded widen the sweep window so horizontal movement
    // follows angled surfaces without the player falling off.
    //   STEP_UP   — max pixels the surface can rise per frame (upward slope)
    //   STEP_DOWN — max pixels the surface can drop per frame (downward slope)
    static constexpr float STEP_UP   = 6.0f;
    static constexpr float STEP_DOWN = 4.0f;

    float checkTop = p.isGrounded() ? newFeet - STEP_UP : prevFeet;
    float checkBot = p.isGrounded() ? newFeet + STEP_DOWN : newFeet;

    if (falling) {
        // Collect platforms from all houses and trees
        auto checkPlatforms = [&](const std::vector<Platform>& platforms) {
            for (const auto& plat : platforms) {
                // X overlap check
                if (pRight <= plat.x1 || pLeft >= plat.x2) continue;

                // Skip if drop is in progress and tier matches
                if (p.isDropping() && plat.tier == p.dropIgnoreTier()) continue;

                // Sweep: feet swept through [checkTop, checkBot]
                if (plat.y >= checkTop && plat.y <= checkBot) {
                    // Pick the highest (smallest Y) platform we crossed
                    if (plat.y < bestPlatformY) {
                        bestPlatformY = plat.y;
                        bestTier      = plat.tier;
                        landedThisFrame = true;
                    }
                }
            }
        };

        for (const auto& h  : m_houses) checkPlatforms(h->platforms());
        for (const auto& te : m_trees)  checkPlatforms(te->platforms());

        // Hard ground floor
        if (!p.isDropping() || p.dropIgnoreTier() != LaneType::Ground) {
            if (newFeet >= GROUND_FLOOR_Y && prevFeet < GROUND_FLOOR_Y + 2.0f) {
                if (GROUND_FLOOR_Y < bestPlatformY) {
                    bestPlatformY   = GROUND_FLOOR_Y;
                    bestTier        = LaneType::Ground;
                    landedThisFrame = true;
                }
            }
            // Clamp to floor even without a sweep crossing (already at floor)
            if (newFeet >= GROUND_FLOOR_Y && !landedThisFrame) {
                bestPlatformY   = GROUND_FLOOR_Y;
                bestTier        = LaneType::Ground;
                landedThisFrame = true;
            }
        }
    }

    if (landedThisFrame) {
        p.landOnPlatform(bestPlatformY, bestTier);
    } else if (!falling) {
        // Ascending — mark not grounded so gravity keeps applying
        p.setGrounded(false);
    } else {
        // Falling but no platform — not grounded
        p.setGrounded(false);
    }
}

// ─── Tree gap generation ──────────────────────────────────────────────────────
void GameWorld::generateTreeGap(float fromX, float gapWidth) {
    if (gapWidth < 10.0f) return;

    std::uniform_int_distribution<int>   countDist(TREE_GAP_MIN_COUNT, TREE_GAP_MAX_COUNT);
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> botYDist(TREE_BOTTOM_Y_MIN, TREE_BOTTOM_Y_MAX);

    int numTrees = countDist(m_rng);

    // Select tree assets and check total width fits
    std::vector<const TreeAsset*> chosen;
    for (int i = 0; i < numTrees; ++i) {
        const TreeAsset* ta = m_houseAssets.selectTree(m_rng);
        if (ta) chosen.push_back(ta);
    }

    // Keep a minimum margin between house edges and the nearest tree
    const float margin = 12.0f;
    float usable = gapWidth - 2.0f * margin;
    if (usable <= 0.0f) return;

    // Reduce count if total width + spacing exceeds usable space
    while (chosen.size() > 1) {
        float total = 0.0f;
        for (auto* ta : chosen) total += ta->pixelWidth;
        total += static_cast<float>(chosen.size() - 1) * TREE_SPACING;
        if (total <= usable) break;
        chosen.pop_back();
    }
    if (chosen.empty()) return;

    // Check single tree fits
    if (chosen.size() == 1 && chosen[0]->pixelWidth > usable) return;

    // Generate bottom-Y values in ascending order
    float prevY = TREE_BOTTOM_Y_MIN;
    std::vector<float> bottomYs;
    for (size_t i = 0; i < chosen.size(); ++i) {
        float lo = prevY;
        float hi = TREE_BOTTOM_Y_MAX;
        if (lo > hi) lo = hi;
        std::uniform_real_distribution<float> yDist(lo, hi);
        float by = yDist(m_rng);
        bottomYs.push_back(by);
        prevY = by;
    }

    // Center tree group in the usable region (inside the margins)
    float totalTreeW = 0.0f;
    for (auto* ta : chosen) totalTreeW += ta->pixelWidth;
    float totalSpacing = static_cast<float>(chosen.size() - 1) * TREE_SPACING;
    float leftover = usable - totalTreeW - totalSpacing;
    float startPad = leftover * 0.5f;

    float curX = fromX + margin + startPad;
    for (size_t i = 0; i < chosen.size(); ++i) {
        int treeIdx = m_treeCount++;
        float tangled = m_config.tangledLightProbability;
        auto te = std::make_shared<TreeEntity>(curX, bottomYs[i], *chosen[i],
                                               treeIdx, tangled);

        // Wire light string callbacks
        for (auto& ls : te->lightStrings()) {
            ls->onDark = [this](int bulbCount, bool chain) {
                m_score.onLightOff(bulbCount);
                m_darkness.onLightOff(bulbCount);
                if (chain) m_score.onChainReaction(1);
            };
            m_lights.push_back(ls);
        }

        m_trees.push_back(te);
        curX += chosen[i]->pixelWidth + TREE_SPACING;
    }
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

    // Seed static star positions once
    std::uniform_real_distribution<float> syDist(0.0f, static_cast<float>(LANE_GROUND_Y) * 0.85f);
    std::uniform_int_distribution<int>    brDist(120, 220);
    m_stars.resize(80);
    for (auto& s : m_stars) {
        s.x          = xDist(m_rng);
        s.y          = syDist(m_rng);
        s.brightness = static_cast<uint8_t>(brDist(m_rng));
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
    // ── Night sky: y=0 (midnight blue) → y=303 (dark grey) ──────────────────
    constexpr int   SKY_BOTTOM  = 303;
    constexpr float RW          = static_cast<float>(RENDER_WIDTH);

    // Draw as a vertical gradient via horizontal line strips (32 bands)
    constexpr int BANDS = 32;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    for (int i = 0; i < BANDS; ++i) {
        float t  = static_cast<float>(i) / static_cast<float>(BANDS - 1);
        // midnight blue: (10, 15, 46)  →  dark grey: (26, 26, 42)
        uint8_t r = static_cast<uint8_t>(10 + t * 16);
        uint8_t g = static_cast<uint8_t>(15 + t * 11);
        uint8_t b = static_cast<uint8_t>(46 + t * -4);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        float y0 = static_cast<float>(i)     * (SKY_BOTTOM + 1.0f) / static_cast<float>(BANDS);
        float y1 = static_cast<float>(i + 1) * (SKY_BOTTOM + 1.0f) / static_cast<float>(BANDS);
        SDL_FRect band{0.f, y0, RW, y1 - y0};
        SDL_RenderFillRectF(renderer, &band);
    }

    // ── Snowy ground: y=304 → y=399, color #7995c4 ───────────────────────────
    SDL_SetRenderDrawColor(renderer, 0x79, 0x95, 0xc4, 255);
    SDL_FRect ground{0.f, static_cast<float>(SKY_BOTTOM + 1),
                     RW,  static_cast<float>(RENDER_HEIGHT - SKY_BOTTOM - 1)};
    SDL_RenderFillRectF(renderer, &ground);
}

void GameWorld::renderMoon(SDL_Renderer* renderer) const {
    // moon sprite is 24x24; place in top-right area
    SpriteRegistry::draw(renderer, "moon",
                         static_cast<float>(RENDER_WIDTH) - 34.f, 6.f);
}

void GameWorld::renderStars(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto& s : m_stars) {
        SDL_SetRenderDrawColor(renderer, s.brightness, s.brightness,
                               static_cast<uint8_t>(s.brightness + 20 > 255 ? 255 : s.brightness + 20),
                               s.brightness);
        SDL_RenderDrawPointF(renderer, s.x, s.y);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// ─── Lighting ────────────────────────────────────────────────────────────────
void GameWorld::renderLighting(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Ambient darkness overlay — deepens as lights are extinguished.
    // Once the meter is full, cap the overlay so new lights remain visible.
    float darkness = m_darkness.darkness();
    float overlayDark = m_darkness.isFull() ? 0.35f : darkness;
    uint8_t ambAlpha = static_cast<uint8_t>(25.0f + overlayDark * 90.0f);
    SDL_SetRenderDrawColor(renderer, 0, 0, 12, ambAlpha);
    SDL_FRect full = {0.0f, 0.0f,
                      static_cast<float>(RENDER_WIDTH),
                      static_cast<float>(RENDER_HEIGHT)};
    SDL_RenderFillRectF(renderer, &full);

    // Additive warm glow for each active light strand
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    // Concentric rect halos: larger = dimmer, smaller = brighter
    static const float   RADII[] = {26.0f, 20.0f, 14.0f, 8.0f, 4.0f};
    static const uint8_t ALP[]   = {  4,     8,    16,   28,   50};

    for (const auto& ls : m_lights) {
        if (ls->isFullyOff()) continue;
        float sx = ls->position.x + ls->width * 0.5f - m_cameraX;
        if (sx < -32.0f || sx > static_cast<float>(RENDER_WIDTH) + 32.0f) continue;
        float sy = ls->position.y;

        for (int i = 0; i < 5; ++i) {
            float r = RADII[i];
            SDL_SetRenderDrawColor(renderer, 255, 210, 120, ALP[i]);
            SDL_FRect glow = {sx - r, sy - r, r * 2.0f, r * 2.0f};
            SDL_RenderFillRectF(renderer, &glow);
        }
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
