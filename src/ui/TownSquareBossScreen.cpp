#include "ui/TownSquareBossScreen.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include "core/HouseAssetLoader.h"
#include "gameplay/TreeEntity.h"
#include "gameplay/threats/Owl.h"
#include "gameplay/threats/Cat.h"
#include "gameplay/threats/Dog.h"
#include "gameplay/Collision.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>

namespace LightsOut {

// ─── Constructor ─────────────────────────────────────────────────────────────

TownSquareBossScreen::TownSquareBossScreen(Game& game)
    : Screen(game)
    , m_rng(static_cast<unsigned>(SDL_GetTicks()))
{
    m_player.setUpgrades(game.upgrades());
    m_player.position.x = m_cameraX + m_player.screenX();

    buildTree();
    spawnThreats();
    initSnow();

    game.audio().playMusic(Music::Level5, true);
}

// ─── Tree & Threats ──────────────────────────────────────────────────────────

void TownSquareBossScreen::buildTree() {
    // Build TreeAsset for tree_large — probe actual sprite dimensions.
    TreeAsset asset;
    asset.name          = "tree_large";
    asset.spritePath    = m_game.resources().assetPath("sprites/tree_large.png");
    asset.maskPath      = m_game.resources().assetPath("sprites/tree_large_mask.png");
    asset.collisionPath = m_game.resources().assetPath("sprites/tree_large_collision.png");

    SDL_Surface* probe = IMG_Load(asset.spritePath.c_str());
    if (probe) {
        asset.pixelWidth  = static_cast<float>(probe->w);
        asset.pixelHeight = static_cast<float>(probe->h);
        SDL_FreeSurface(probe);
    } else {
        asset.pixelWidth  = 177.0f;
        asset.pixelHeight = 275.0f;
    }

    // Create a temporary TreeEntity to parse platforms and light strands.
    // tangledProb = 0.35 so roughly a third of strands need multiple bites.
    TreeEntity tree(TREE_WORLD_X, LANE_GROUND_Y + 20.0f, asset, 999, 0.35f);

    // Store runtime dimensions and platforms.
    m_treeSpriteW   = asset.pixelWidth;
    m_treeSpriteH   = asset.pixelHeight;
    m_treeSpriteY   = LANE_GROUND_Y + 20.0f - asset.pixelHeight;
    m_treePlatforms = tree.platforms();

    // Wire every light strand from the tree into m_lights for bite detection.
    for (auto& ls : tree.lightStrings()) {
        ls->onDark = [](int, bool) {};   // scoring happens on YouWin
        m_lights.push_back(ls);
    }
    m_totalLights = static_cast<int>(m_lights.size());
}

void TownSquareBossScreen::spawnThreats() {
    // tree_large is ~177px wide.  Threats are positioned relative to its flanks.
    const float treeRight = TREE_WORLD_X + m_treeSpriteW;

    // Owls on fence tier — one on each side just beyond the canopy edge
    m_threats.push_back(std::make_shared<Owl>(
        treeRight + 35.0f, LANE_FENCE_Y - 14.0f));
    m_threats.push_back(std::make_shared<Owl>(
        TREE_WORLD_X - 45.0f, LANE_FENCE_Y - 14.0f));

    // Cats on rooftop — patrol the upper canopy flanks
    m_threats.push_back(std::make_shared<Cat>(
        treeRight + 28.0f, LANE_ROOFTOP_Y - 7.0f));
    m_threats.push_back(std::make_shared<Cat>(
        TREE_WORLD_X - 38.0f, LANE_ROOFTOP_Y - 7.0f));

    // Dogs on ground — wide chain radius to cover the full tree base
    m_threats.push_back(std::make_shared<Dog>(
        treeRight + 60.0f, LANE_GROUND_Y - 8.0f, 60.0f));
    m_threats.push_back(std::make_shared<Dog>(
        TREE_WORLD_X - 60.0f, LANE_GROUND_Y - 8.0f, 60.0f));
}

// ─── Snow ─────────────────────────────────────────────────────────────────────

void TownSquareBossScreen::initSnow() {
    std::uniform_real_distribution<float> xd(0.0f, static_cast<float>(RENDER_WIDTH));
    std::uniform_real_distribution<float> yd(0.0f, static_cast<float>(RENDER_HEIGHT));
    std::uniform_real_distribution<float> sd(SNOW_FALL_SPEED * 0.7f, SNOW_FALL_SPEED * 1.3f);
    std::uniform_real_distribution<float> dd(-0.5f, 0.5f);
    m_snow.resize(SNOW_PARTICLES);
    for (auto& f : m_snow) {
        f.pos   = {xd(m_rng), yd(m_rng)};
        f.speed = sd(m_rng);
        f.drift = dd(m_rng);
    }
}

void TownSquareBossScreen::updateSnow(float dt) {
    for (auto& f : m_snow) {
        f.pos.y += f.speed * dt;
        f.pos.x += f.drift;
        if (f.pos.y > RENDER_HEIGHT) f.pos.y = -2.0f;
        if (f.pos.x < 0) f.pos.x += RENDER_WIDTH;
        if (f.pos.x > RENDER_WIDTH) f.pos.x -= RENDER_WIDTH;
    }
}

// ─── Input ───────────────────────────────────────────────────────────────────

void TownSquareBossScreen::handleInput() {
    if (m_won || m_deathTimer > 0.0f) return;

    auto& input = m_game.input();
    if (input.isActionJustPressed(Action::Pause)) {
        m_game.pushState(GameState::Paused);
        return;
    }

    if (input.isActionJustPressed(Action::Jump))  m_player.jump();
    if (input.isActionJustPressed(Action::Drop))  m_player.drop();
    if (input.isActionJustPressed(Action::Bite)) {
        m_player.tryBite(m_lights, m_cameraX);
        // Remove fully-extinguished strings immediately
        m_lights.erase(
            std::remove_if(m_lights.begin(), m_lights.end(),
                [](const std::shared_ptr<LightString>& ls) { return ls->isFullyOff(); }),
            m_lights.end());
    }
    if (input.isActionJustPressed(Action::UsePowerUp)) {
        // Power-ups collected in prior levels carry over
        // (boss screen doesn't spawn new ones, but held ones still work)
    }
}

// ─── Update ──────────────────────────────────────────────────────────────────

void TownSquareBossScreen::update(float dt) {
    // Win transition — play out then go to YouWin
    if (m_won) {
        m_winTimer += dt;
        if (m_winTimer >= 3.5f) {
            m_game.addScore(5000);   // completion bonus
            m_game.replaceState(GameState::YouWin);
        }
        return;
    }

    // Death overlay countdown
    if (m_deathTimer > 0.0f) {
        m_deathTimer -= dt;
        if (m_deathTimer <= 0.0f) {
            m_deathTimer = 0.0f;
            if (m_pendingRespawn) {
                m_pendingRespawn = false;
                doRespawn();
            } else {
                m_game.resetLives();
                m_game.audio().stopMusic();
                m_game.audio().playSfx(SoundEffect::GameOver);
                m_game.replaceState(GameState::GameOver);
            }
        }
        return;   // world frozen during overlay
    }

    // Scroll camera toward the lock point
    if (!m_camLocked) {
        m_cameraX += SCROLL_SPEED * dt;
        if (m_cameraX >= LOCK_CAM_X) {
            m_cameraX  = LOCK_CAM_X;
            m_camLocked = true;
        }
    }

    // Sync player world position with screen X
    m_player.position.x = m_cameraX + m_player.screenX();

    // Horizontal movement (held keys)
    auto& input = m_game.input();
    float hdir = 0.0f;
    if (input.isActionDown(Action::MoveLeft))  hdir -= 1.0f;
    if (input.isActionDown(Action::MoveRight)) hdir += 1.0f;
    if (hdir != 0.0f) m_player.moveHorizontal(hdir, dt);

    m_player.update(dt);
    m_player.tickDropTimer(dt);
    resolvePlatformCollision();
    m_particles.setCameraX(m_cameraX);
    m_particles.update(dt);

    // Update light strings (cascade animations)
    for (auto& ls : m_lights) ls->update(dt);

    // Prune fully-dark strings between bites (cascade completes on non-bite frames)
    m_lights.erase(
        std::remove_if(m_lights.begin(), m_lights.end(),
            [](const std::shared_ptr<LightString>& ls) { return ls->isFullyOff(); }),
        m_lights.end());

    // Update threats — alert only once they're on-screen
    const Vec2 playerWorld = {m_player.position.x, laneY(m_player.currentLane())};
    for (auto& t : m_threats) {
        if (auto* owl = dynamic_cast<Owl*>(t.get())) {
            bool onFence = (m_player.currentLane() == LaneType::Fence &&
                            std::abs(m_player.position.x - owl->position.x) < 60.0f);
            owl->playerOnFence(onFence);
        }
        // Alert when the threat scrolls into view
        if (!t->isAlerted()) {
            float sx = t->position.x - m_cameraX;
            if (sx > -16.0f && sx < static_cast<float>(RENDER_WIDTH) + 16.0f)
                t->alert(playerWorld);
        }
        if (t->isAlerted()) t->alert(playerWorld);   // keep target updated
        t->update(dt);
    }

    updateSnow(dt);
    checkCollisions();

    // Win condition: all light strings extinguished
    if (m_lights.empty() && m_totalLights > 0) {
        m_won      = true;
        m_winTimer = 0.0f;
        m_game.audio().stopMusic();
        m_game.audio().playSfx(SoundEffect::StageComplete);
    }
}

// ─── Collision ───────────────────────────────────────────────────────────────

void TownSquareBossScreen::checkCollisions() {
    if (m_player.isInvincible() || m_player.isShadowMode()) return;
    Rect pb = m_player.bounds();
    for (auto& t : m_threats) {
        if (!t->alive) continue;
        if (t->caughtPlayer(pb)) {
            onPlayerDeath();
            return;
        }
    }
}

int TownSquareBossScreen::countLightsRemaining() const {
    int n = 0;
    for (const auto& ls : m_lights)
        if (!ls->isFullyOff()) ++n;
    return n;
}

void TownSquareBossScreen::doRespawn() {
    m_player.respawn();
    m_player.position.x = m_cameraX + m_player.screenX();
    for (auto& t : m_threats) t->freeze(RESPAWN_THREAT_FREEZE);
}

void TownSquareBossScreen::onPlayerDeath() {
    m_game.decrementLives();
    m_pendingRespawn = (m_game.lives() > 0);
    m_deathTimer     = DEATH_OVERLAY_DURATION;
}

// ─── Platform collision ───────────────────────────────────────────────────────

void TownSquareBossScreen::resolvePlatformCollision() {
    Player& p = m_player;
    if (p.state() == PlayerState::Dead) return;

    const float prevFeet = p.prevFeetY();
    const float newFeet  = p.position.y + p.height;
    const float pLeft    = p.position.x;
    const float pRight   = p.position.x + p.width;

    bool falling = (p.velocityY() >= 0.0f);

    bool landedThisFrame = false;
    float bestPlatformY  = 1e9f;
    LaneType bestTier    = LaneType::Ground;

    static constexpr float STEP_UP   = 6.0f;
    static constexpr float STEP_DOWN = 4.0f;
    float checkTop = p.isGrounded() ? newFeet - STEP_UP  : prevFeet;
    float checkBot = p.isGrounded() ? newFeet + STEP_DOWN : newFeet;

    if (falling) {
        for (const auto& plat : m_treePlatforms) {
            if (pRight <= plat.x1 || pLeft >= plat.x2) continue;
            if (p.isDropping() && plat.tier == p.dropIgnoreTier()) continue;
            if (plat.y >= checkTop && plat.y <= checkBot) {
                if (plat.y < bestPlatformY) {
                    bestPlatformY   = plat.y;
                    bestTier        = plat.tier;
                    landedThisFrame = true;
                }
            }
        }

        // Hard ground floor
        if (!p.isDropping() || p.dropIgnoreTier() != LaneType::Ground) {
            if (newFeet >= GROUND_FLOOR_Y) {
                if (GROUND_FLOOR_Y < bestPlatformY) {
                    bestPlatformY   = GROUND_FLOOR_Y;
                    bestTier        = LaneType::Ground;
                    landedThisFrame = true;
                }
                if (!landedThisFrame) {
                    bestPlatformY   = GROUND_FLOOR_Y;
                    bestTier        = LaneType::Ground;
                    landedThisFrame = true;
                }
            }
        }
    }

    if (landedThisFrame)
        p.landOnPlatform(bestPlatformY, bestTier);
    else
        p.setGrounded(false);
}

// ─── Render ──────────────────────────────────────────────────────────────────

void TownSquareBossScreen::render() {
    SDL_Renderer* r = m_game.renderer().sdl();

    renderBackground(r);
    renderTree(r);   // also renders all light strands

    for (auto& t  : m_threats)  t->render(r, m_cameraX);
    m_player.render(r, m_cameraX);
    m_particles.render(r);

    renderLaneGuides(r);
    renderSnow(r);
    drawHUD(r);

    if (m_deathTimer > 0.0f) drawDeathOverlay(r);
    if (m_won)               drawWinTransition(r);
}

void TownSquareBossScreen::renderBackground(SDL_Renderer* r) const {
    int skyH = static_cast<int>(LANE_GROUND_Y);
    for (int tx = 0; tx < RENDER_WIDTH; tx += 64)
        SpriteRegistry::draw(r, "sky_gradient",
                             static_cast<float>(tx), 0.f, 64.f, static_cast<float>(skyH));
    for (int tx = 0; tx < RENDER_WIDTH; tx += 64)
        SpriteRegistry::draw(r, "ground_strip",
                             static_cast<float>(tx), LANE_GROUND_Y,
                             64.f, static_cast<float>(RENDER_HEIGHT - skyH));
    for (int tx = 0; tx < RENDER_WIDTH; tx += 64)
        SpriteRegistry::draw(r, "stars_overlay",
                             static_cast<float>(tx), 0.f, 64.f, static_cast<float>(skyH), 128);
    SpriteRegistry::draw(r, "moon",
                         static_cast<float>(RENDER_WIDTH) - 34.f, 6.f);
}

void TownSquareBossScreen::renderTree(SDL_Renderer* r) const {
    float sx = TREE_WORLD_X - m_cameraX;

    SpriteRegistry::draw(r, "tree_large", sx, m_treeSpriteY, m_treeSpriteW, m_treeSpriteH);

    // Soft festive glow around the tree
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 200, 60, 15);
    SDL_FRect glow = {sx - 12.0f, m_treeSpriteY, m_treeSpriteW + 24.0f, m_treeSpriteH};
    SDL_RenderFillRectF(r, &glow);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // Render light strands stored in m_lights
    for (const auto& ls : m_lights) ls->render(r, m_cameraX);
}

void TownSquareBossScreen::renderLaneGuides(SDL_Renderer* r) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 100, 100, 140, 30);
    SDL_RenderDrawLineF(r, 0, LANE_ROOFTOP_Y, static_cast<float>(RENDER_WIDTH), LANE_ROOFTOP_Y);
    SDL_RenderDrawLineF(r, 0, LANE_FENCE_Y,   static_cast<float>(RENDER_WIDTH), LANE_FENCE_Y);
    SDL_SetRenderDrawColor(r, 60, 80, 40, 80);
    SDL_RenderDrawLineF(r, 0, LANE_GROUND_Y,  static_cast<float>(RENDER_WIDTH), LANE_GROUND_Y);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void TownSquareBossScreen::renderSnow(SDL_Renderer* r) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 220, 235, 255, 180);
    for (const auto& f : m_snow) {
        SDL_FRect flake = {f.pos.x, f.pos.y, 1.0f, 2.0f};
        SDL_RenderFillRectF(r, &flake);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void TownSquareBossScreen::drawHUD(SDL_Renderer* r) const {
    (void)r;
    auto& renderer = m_game.renderer();

    renderer.drawText("TOWN SQUARE", {RENDER_WIDTH * 0.5f, 2.0f},
                       {180, 100, 100}, 8, true);

    // Lights remaining counter
    int rem = countLightsRemaining();
    Color remCol = rem > 0 ? Color{255, 220, 50} : Color{100, 255, 100};
    renderer.drawText(std::to_string(rem) + " LIGHTS LEFT",
                       {RENDER_WIDTH * 0.5f, 19.0f}, remCol, 8, true);

    renderer.drawText("LIVES " + std::to_string(m_game.lives()),
                       {2.0f, 2.0f}, Color::White());
    renderer.drawText("SCORE " + std::to_string(m_game.totalScore()),
                       {2.0f, 19.0f}, {180, 180, 200});
}

void TownSquareBossScreen::drawDeathOverlay(SDL_Renderer* r) const {
    float progress = 1.0f - (m_deathTimer / DEATH_OVERLAY_DURATION);
    uint8_t alpha = static_cast<uint8_t>(std::min(1.0f, progress * 2.0f) * 200.0f);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, alpha);
    SDL_FRect full = {0.0f, 0.0f,
                      static_cast<float>(RENDER_WIDTH), static_cast<float>(RENDER_HEIGHT)};
    SDL_RenderFillRectF(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    if (progress > 0.4f) {
        m_game.renderer().drawText("WIPEOUT!",
            {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.40f}, {255, 80, 80}, 12, true);
        if (m_pendingRespawn) {
            m_game.renderer().drawText(
                std::to_string(m_game.lives()) + " LIVES LEFT",
                {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.55f}, {200, 200, 200}, 8, true);
        } else {
            m_game.renderer().drawText("GAME OVER",
                {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.55f}, {200, 80, 80}, 8, true);
        }
    }
}

void TownSquareBossScreen::drawWinTransition(SDL_Renderer* r) const {
    float progress = m_winTimer / 3.5f;
    uint8_t alpha = static_cast<uint8_t>(std::min(1.0f, progress * 1.5f) * 220.0f);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, alpha);
    SDL_FRect full = {0.0f, 0.0f,
                      static_cast<float>(RENDER_WIDTH), static_cast<float>(RENDER_HEIGHT)};
    SDL_RenderFillRectF(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    if (m_winTimer > 1.0f) {
        float ta = std::min(1.0f, (m_winTimer - 1.0f) * 2.0f);
        uint8_t ta8 = static_cast<uint8_t>(ta * 255.0f);
        m_game.renderer().drawText("LIGHTS OUT!",
            {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.38f},
            {255, 220, 50, ta8}, 12, true);
        m_game.renderer().drawText("The tree goes dark...",
            {RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.56f},
            {180, 200, 255, ta8}, 8, true);
    }
}

}  // namespace LightsOut
