#include "core/SpriteRegistry.h"
#include "core/ResourceManager.h"
#include <string>

namespace LightsOut {

ResourceManager* SpriteRegistry::s_rm = nullptr;

SDL_Texture* SpriteRegistry::get(const char* name, int* outW, int* outH) {
    if (!s_rm) return nullptr;
    std::string path = std::string("sprites/") + name + ".png";
    SDL_Texture* tex = s_rm->getTexture(path);
    if (tex && (outW || outH)) {
        int w = 0, h = 0;
        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        if (outW) *outW = w;
        if (outH) *outH = h;
    }
    return tex;
}

void SpriteRegistry::draw(SDL_Renderer* r, const char* name,
                           float x, float y,
                           float scaleW, float scaleH,
                           uint8_t alpha, SDL_RendererFlip flip) {
    int tw = 0, th = 0;
    SDL_Texture* tex = get(name, &tw, &th);
    if (!tex || tw == 0 || th == 0) return;

    float dw = (scaleW > 0.f) ? scaleW : static_cast<float>(tw);
    float dh = (scaleH > 0.f) ? scaleH : static_cast<float>(th);

    SDL_SetTextureAlphaMod(tex, alpha);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    SDL_Rect  src = {0, 0, tw, th};
    SDL_FRect dst = {x, y, dw, dh};
    SDL_RenderCopyExF(r, tex, &src, &dst, 0.0, nullptr, flip);

    SDL_SetTextureAlphaMod(tex, 255);
}

}  // namespace LightsOut
