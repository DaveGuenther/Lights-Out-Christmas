#include "gameplay/House.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <random>
#include <algorithm>

namespace LightsOut {

static std::mt19937 s_houseRng(123);

House::House(float worldX, float worldWidth, HouseStyle style,
             int houseIndex, int roofStrands, int windowStrands, int porchStrands,
             float tangledProb)
    : Entity(TAG_HOUSE)
    , m_houseIndex(houseIndex)
    , m_style(style)
{
    position.x = worldX;
    position.y = HOUSE_GROUND_Y - HOUSE_HEIGHT;
    width  = worldWidth;
    height = HOUSE_HEIGHT;

    // Random palette per house
    static const Color walls[]  = {{50,50,65},{45,35,30},{30,50,45},{55,45,35}};
    static const Color roofs[]  = {{35,25,20},{25,35,45},{40,25,40},{30,40,25}};
    std::uniform_int_distribution<int> wd(0,3), rd(0,3);
    wallColor = walls[wd(s_houseRng)];
    roofColor = roofs[rd(s_houseRng)];

    if (roofStrands   > 0) placeRoofLights(roofStrands, tangledProb);
    if (windowStrands > 0) placeWindowLights(windowStrands, tangledProb);
    if (porchStrands  > 0) placePorchLights(porchStrands, tangledProb);
}

void House::update(float dt) {
    for (auto& ls : m_lightStrings) ls->update(dt);

    if (m_porchResetting) {
        m_porchResetTimer -= dt;
        if (m_porchResetTimer <= 0.0f) {
            m_porchLightOn  = false;
            m_porchResetting = false;
        }
    }
}

void House::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    drawHouseBody(renderer, sx);
    drawPorchLight(renderer, sx);

    for (auto& ls : m_lightStrings) ls->render(renderer, cameraX);
}

bool House::isFullyDark() const {
    for (const auto& ls : m_lightStrings)
        if (!ls->isFullyOff()) return false;
    return !m_lightStrings.empty();
}

Vec2 House::porchLightPos() const {
    return {position.x + width * 0.15f, HOUSE_GROUND_Y - 15.0f};
}

void House::triggerPorchLight() {
    m_porchLightOn  = true;
    m_porchResetting = false;
}

void House::resetPorchLight(float delaySeconds) {
    m_porchResetTimer = delaySeconds;
    m_porchResetting  = true;
}

void House::placeRoofLights(int count, float tangledProb) {
    if (count <= 0) return;
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);

    float segW = width / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        float lx = position.x + static_cast<float>(i) * segW;
        float ly = position.y + 2.0f;  // along roofline
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(lx, ly, segW - 2.0f, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::placeWindowLights(int count, float tangledProb) {
    if (count <= 0) return;
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);

    // Window-ledge lights sit just below the window row (~20px below house top)
    float windowY = position.y + 22.0f;
    float segW    = width / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        float lx = position.x + static_cast<float>(i) * segW;
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(lx, windowY, segW - 2.0f, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::placePorchLights(int count, float tangledProb) {
    if (count <= 0) return;
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);

    float porchY  = HOUSE_GROUND_Y - 22.0f;
    float porchW  = width * 0.6f;
    float startX  = position.x + width * 0.2f;
    float segW    = porchW / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        float lx = startX + static_cast<float>(i) * segW;
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(lx, porchY, segW - 2.0f, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::drawHouseBody(SDL_Renderer* renderer, float screenX) const {
    // Wall
    SDL_SetRenderDrawColor(renderer, wallColor.r, wallColor.g, wallColor.b, 255);
    SDL_FRect wall = {screenX, position.y + 10.0f, width, height - 10.0f};
    SDL_RenderFillRectF(renderer, &wall);

    // Roof (flat top rect slightly darker)
    SDL_SetRenderDrawColor(renderer, roofColor.r, roofColor.g, roofColor.b, 255);
    SDL_FRect roof = {screenX - 2.0f, position.y, width + 4.0f, 14.0f};
    SDL_RenderFillRectF(renderer, &roof);

    // Snow on roof
    SDL_SetRenderDrawColor(renderer, 220, 235, 255, 200);
    SDL_FRect snow = {screenX - 2.0f, position.y, width + 4.0f, 4.0f};
    SDL_RenderFillRectF(renderer, &snow);

    // Chimney
    SDL_SetRenderDrawColor(renderer, roofColor.r + 10, roofColor.g + 10, roofColor.b + 10, 255);
    SDL_FRect chimney = {screenX + width * 0.75f, position.y - 8.0f, 6.0f, 12.0f};
    SDL_RenderFillRectF(renderer, &chimney);

    // Windows (2 small yellow squares)
    SDL_SetRenderDrawColor(renderer, 255, 200, 80, 200);
    SDL_FRect win1 = {screenX + width * 0.15f, position.y + 20.0f, 8.0f, 8.0f};
    SDL_FRect win2 = {screenX + width * 0.65f, position.y + 20.0f, 8.0f, 8.0f};
    SDL_RenderFillRectF(renderer, &win1);
    SDL_RenderFillRectF(renderer, &win2);

    // Window cross bars
    SDL_SetRenderDrawColor(renderer, wallColor.r + 20, wallColor.g + 20, wallColor.b + 20, 255);
    SDL_RenderDrawLineF(renderer, screenX + width * 0.15f + 4.0f, position.y + 20.0f,
                                  screenX + width * 0.15f + 4.0f, position.y + 28.0f);
    SDL_RenderDrawLineF(renderer, screenX + width * 0.15f,         position.y + 24.0f,
                                  screenX + width * 0.15f + 8.0f,  position.y + 24.0f);

    // Door
    SDL_SetRenderDrawColor(renderer, roofColor.r + 15, roofColor.g, roofColor.b, 255);
    SDL_FRect door = {screenX + width * 0.42f, position.y + height - 28.0f, 10.0f, 18.0f};
    SDL_RenderFillRectF(renderer, &door);

    // Door knob
    SDL_SetRenderDrawColor(renderer, 200, 180, 50, 255);
    SDL_RenderDrawPointF(renderer, screenX + width * 0.42f + 8.0f, position.y + height - 20.0f);
}

void House::drawPorchLight(SDL_Renderer* renderer, float screenX) const {
    Vec2 lp = porchLightPos();
    float lpx = lp.x - (position.x - screenX) - position.x + screenX;
    // Simpler: screenX is already offset
    lpx = screenX + width * 0.15f;
    float lpy = HOUSE_GROUND_Y - 15.0f;

    // Fixture
    SDL_SetRenderDrawColor(renderer, 180, 180, 100, 255);
    SDL_FRect fixture = {lpx - 1.5f, lpy - 2.0f, 3.0f, 4.0f};
    SDL_RenderFillRectF(renderer, &fixture);

    if (m_porchLightOn) {
        // Light cone
        SDL_SetRenderDrawColor(renderer, 255, 230, 120, 60);
        for (int i = 0; i < 5; ++i) {
            SDL_FRect cone = {lpx - static_cast<float>(i) * 3.0f,
                              lpy + static_cast<float>(i) * 4.0f,
                              static_cast<float>(i) * 6.0f + 2.0f,
                              4.0f};
            SDL_RenderFillRectF(renderer, &cone);
        }
        // Bulb glow
        SDL_SetRenderDrawColor(renderer, 255, 230, 120, 200);
        SDL_FRect bulb = {lpx - 2.0f, lpy - 1.0f, 4.0f, 4.0f};
        SDL_RenderFillRectF(renderer, &bulb);
    } else {
        // Dim fixture
        SDL_SetRenderDrawColor(renderer, 80, 80, 50, 180);
        SDL_FRect bulb = {lpx - 1.0f, lpy, 2.0f, 2.0f};
        SDL_RenderFillRectF(renderer, &bulb);
    }
}

}  // namespace LightsOut
