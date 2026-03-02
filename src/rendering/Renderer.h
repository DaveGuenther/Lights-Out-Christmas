#pragma once
#include "core/Types.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace LightsOut {

// Wraps SDL_Renderer with pixel-art scaling support.
// All game code renders at RENDER_WIDTH x RENDER_HEIGHT (320x200).
// The Renderer scales up to SCREEN_WIDTH x SCREEN_HEIGHT (1280x800) for display.
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(SDL_Window* window);
    void shutdown();

    // Call before rendering each frame
    void beginFrame();
    // Call after rendering; blits the render target to the screen at PIXEL_SCALE
    void endFrame();

    // Sets draw color
    void setColor(Color c);
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Render-space drawing (coordinates in 0..RENDER_WIDTH / 0..RENDER_HEIGHT)
    void clear(Color c = {8, 12, 35});
    void drawRect(const Rect& r);
    void fillRect(const Rect& r);
    void drawLine(Vec2 a, Vec2 b);
    void drawPoint(Vec2 p);
    void fillCircle(Vec2 center, float radius);

    // Text rendering (returns rendered width in render-space pixels)
    int drawText(const std::string& text, Vec2 pos, Color c,
                 int fontSize = 8, bool center = false);

    // Texture drawing from render-target (used for cached sprites)
    void drawTexture(SDL_Texture* tex, const Rect& src, const Rect& dst);

    // Access raw SDL renderer (for entities that draw themselves)
    SDL_Renderer* sdl() { return m_renderer; }

    // For use inside entity render() calls: convert world X to screen X
    static float worldToScreen(float worldX, float cameraX) {
        return worldX - cameraX;
    }

    // Pixel-art render target (everything renders here first)
    SDL_Texture* renderTarget() { return m_renderTarget; }

private:
    SDL_Renderer* m_renderer     = nullptr;
    SDL_Texture*  m_renderTarget = nullptr;  // 320x200 render target
    SDL_Texture*  m_font         = nullptr;  // built-in bitmap font
    bool          m_initialized  = false;

    void loadBitmapFont();
    void drawChar(char c, int x, int y, Color col);
};

}  // namespace LightsOut
