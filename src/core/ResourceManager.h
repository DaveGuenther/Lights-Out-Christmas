#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace LightsOut {

class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager();

    bool init(SDL_Renderer* renderer);
    void shutdown();

    // Returns cached texture; loads from file on first call.
    // Returns nullptr if file not found (callers should handle gracefully).
    SDL_Texture* getTexture(const std::string& path);

    // Font access (returns nullptr if SDL_ttf failed)
    TTF_Font* getFont(int size);

    // Asset base path (set to directory of executable)
    void setBasePath(const std::string& path);
    const std::string& basePath() const { return m_basePath; }

    // Build full asset path
    std::string assetPath(const std::string& relative) const;

private:
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> m_textures;
    std::unordered_map<int, TTF_Font*>            m_fonts;
    std::string m_basePath;
    bool m_initialized = false;
};

}  // namespace LightsOut
