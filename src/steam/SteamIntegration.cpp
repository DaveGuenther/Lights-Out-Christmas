#include "steam/SteamIntegration.h"
#include <SDL2/SDL.h>
#include <vector>
#include <cstring>

namespace LightsOut {

SteamIntegration& SteamIntegration::instance() {
    static SteamIntegration inst;
    return inst;
}

bool SteamIntegration::init() {
#ifdef STEAM_ENABLED
    // SteamAPI_Init() would go here
    // if (!SteamAPI_Init()) { ... return false; }
    m_available = true;
    SDL_Log("Steam integration initialized.");
#else
    m_available = false;
    SDL_Log("Steam integration: running in offline mode (STEAM_ENABLED not set).");
#endif
    return true;  // Non-fatal either way
}

void SteamIntegration::shutdown() {
#ifdef STEAM_ENABLED
    if (m_available) {
        // SteamAPI_Shutdown();
        m_available = false;
    }
#endif
}

void SteamIntegration::runCallbacks() {
#ifdef STEAM_ENABLED
    if (m_available) {
        // SteamAPI_RunCallbacks();
    }
#endif
}

void SteamIntegration::submitScore(int score,
                                    std::function<void(bool)> cb) {
    if (!m_available) {
        if (cb) cb(false);
        return;
    }
#ifdef STEAM_ENABLED
    // SteamUserStats()->UploadLeaderboardScore(...)
    // For now, just succeed locally
    (void)score;
    if (cb) cb(true);
#else
    (void)score;
    if (cb) cb(false);
#endif
}

void SteamIntegration::fetchLeaderboard(int maxEntries,
                                         std::function<void(bool)> cb) {
    (void)maxEntries;
    m_entries.clear();
    if (!m_available) {
        if (cb) cb(false);
        return;
    }
    // Stub: return empty leaderboard
    if (cb) cb(true);
}

void SteamIntegration::unlockAchievement(const char* apiName) {
    if (!m_available) return;
    SDL_Log("Achievement unlocked: %s", apiName);
#ifdef STEAM_ENABLED
    // SteamUserStats()->SetAchievement(apiName);
    // SteamUserStats()->StoreStats();
#endif
}

bool SteamIntegration::hasAchievement(const char* apiName) const {
    (void)apiName;
    return false;
}

void SteamIntegration::setStatInt(const char* name, int value) {
    if (!m_available) return;
    SDL_Log("Stat set: %s = %d", name, value);
#ifdef STEAM_ENABLED
    // SteamUserStats()->SetStat(name, value);
#endif
}

void SteamIntegration::setStatFloat(const char* name, float value) {
    if (!m_available) return;
    SDL_Log("Stat set: %s = %.2f", name, value);
#ifdef STEAM_ENABLED
    // SteamUserStats()->SetStat(name, value);
#endif
}

void SteamIntegration::storeStats() {
    if (!m_available) return;
#ifdef STEAM_ENABLED
    // SteamUserStats()->StoreStats();
#endif
}

bool SteamIntegration::saveToCloud(const std::string& filename,
                                    const void* data, int size) {
    if (!m_available) return false;
#ifdef STEAM_ENABLED
    // SteamRemoteStorage()->FileWrite(filename.c_str(), data, size);
    (void)filename; (void)data; (void)size;
    return true;
#else
    (void)filename; (void)data; (void)size;
    return false;
#endif
}

bool SteamIntegration::loadFromCloud(const std::string& filename,
                                      void* outData, int maxSize,
                                      int& outBytesRead) {
    outBytesRead = 0;
    if (!m_available) return false;
#ifdef STEAM_ENABLED
    // outBytesRead = SteamRemoteStorage()->FileRead(filename.c_str(), outData, maxSize);
    (void)filename; (void)outData; (void)maxSize;
    return outBytesRead > 0;
#else
    (void)filename; (void)outData; (void)maxSize;
    return false;
#endif
}

}  // namespace LightsOut
