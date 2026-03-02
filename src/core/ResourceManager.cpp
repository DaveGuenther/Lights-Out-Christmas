#include "core/ResourceManager.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>
#include <algorithm>

namespace LightsOut {

ResourceManager::~ResourceManager() {
    shutdown();
}

bool ResourceManager::init(SDL_Renderer* renderer) {
    m_renderer = renderer;

    if (TTF_Init() < 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "SDL_ttf init failed: %s (continuing without fonts)", TTF_GetError());
    }

    char* base = SDL_GetBasePath();
    if (base) {
        m_basePath = base;
        SDL_free(base);
    } else {
        m_basePath = "./";
    }

    m_initialized = true;
    return true;
}

void ResourceManager::shutdown() {
    for (auto& [path, tex] : m_textures) {
        if (tex) SDL_DestroyTexture(tex);
    }
    m_textures.clear();

    for (auto& [size, font] : m_fonts) {
        if (font) TTF_CloseFont(font);
    }
    m_fonts.clear();

    if (m_initialized) {
        TTF_Quit();
        m_initialized = false;
    }
}

SDL_Texture* ResourceManager::getTexture(const std::string& path) {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) return it->second;

    std::string fullPath = assetPath(path);
    SDL_Texture* tex = IMG_LoadTexture(m_renderer, fullPath.c_str());

    if (!tex) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Texture not found: %s — using placeholder", fullPath.c_str());
        // Create 16x16 magenta placeholder
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32,
                                                           SDL_PIXELFORMAT_RGBA8888);
        if (surf) {
            SDL_FillRect(surf, nullptr, SDL_MapRGB(surf->format, 255, 0, 255));
            tex = SDL_CreateTextureFromSurface(m_renderer, surf);
            SDL_FreeSurface(surf);
        }
    }

    m_textures[path] = tex;
    return tex;
}

TTF_Font* ResourceManager::getFont(int size) {
    auto it = m_fonts.find(size);
    if (it != m_fonts.end()) return it->second;

    // Candidate font paths
    std::vector<std::string> candidates = {
        assetPath("fonts/main.ttf"),
#ifdef _WIN32
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
#endif
    };

    TTF_Font* font = nullptr;
    for (const auto& path : candidates) {
        font = TTF_OpenFont(path.c_str(), size);
        if (font) break;
    }

    if (!font) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "No font found for size %d — text rendering unavailable", size);
    }

    m_fonts[size] = font;
    return font;
}

void ResourceManager::setBasePath(const std::string& path) {
    m_basePath = path;
    if (!m_basePath.empty() && m_basePath.back() != '/' && m_basePath.back() != '\\')
        m_basePath += '/';
}

std::string ResourceManager::assetPath(const std::string& relative) const {
    return m_basePath + "assets/" + relative;
}

}  // namespace LightsOut
