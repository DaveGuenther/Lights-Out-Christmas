#pragma once
#include <SDL2/SDL.h>

namespace LightsOut {

class ResourceManager;

// Lightweight service-locator for sprite textures.
// Call SpriteRegistry::init() once after ResourceManager::init().
// All entity render() methods can then call SpriteRegistry::draw() without
// carrying extra parameters through the virtual render() interface.
class SpriteRegistry {
public:
    static void init(ResourceManager* rm) { s_rm = rm; }

    // Draw named sprite at render-space position (x, y).
    // scaleW / scaleH == 0 → use the sprite's natural pixel dimensions.
    // alpha  0-255 (255 = fully opaque)
    // flip   SDL_FLIP_NONE / SDL_FLIP_HORIZONTAL / SDL_FLIP_VERTICAL
    static void draw(SDL_Renderer*    r,
                     const char*      name,
                     float            x,
                     float            y,
                     float            scaleW  = 0.f,
                     float            scaleH  = 0.f,
                     uint8_t          alpha   = 255,
                     SDL_RendererFlip flip    = SDL_FLIP_NONE);

    // Returns the SDL_Texture* for the given sprite name (nullptr if missing).
    // Optionally writes natural pixel dimensions to outW / outH.
    static SDL_Texture* get(const char* name,
                            int*        outW = nullptr,
                            int*        outH = nullptr);

private:
    static ResourceManager* s_rm;
};

}  // namespace LightsOut
