#pragma once
#include "core/ChiptunePlayer.h"
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>

namespace LightsOut {

enum class SoundEffect {
    LightPop,        // single bulb pops
    LightCascade,    // whole strand going dark
    Chomp,           // squirrel biting
    HomeownerAlert,  // "Hey! Get outta there!"
    DogBark,
    PowerUpCollect,
    PowerUpEnd,
    LaneJump,
    Spark,
    StageComplete,
    GameOver,
    ComboChime,
    Count
};

enum class Music {
    MainTheme,
    Level1,   // Suburban (calm)
    Level2,   // Rich Neighborhood (upbeat)
    Level3,   // Cul-de-Sac (eerie loop)
    Level4,   // Christmas Eve (intense)
    Level5,   // Town Square (boss)
    Count
};

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    bool init();
    void shutdown();

    void playSfx(SoundEffect sfx, int volume = MIX_MAX_VOLUME);
    void playMusic(Music music, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

    // Dynamically adjust BGM tempo based on chaos level (0.0–1.0)
    void setMusicIntensity(float intensity);

    void setMasterVolume(int vol);   // 0–128
    void setSfxVolume(int vol);
    void setMusicVolume(int vol);

    bool isMusicPlaying() const;

    // Load from file; if file not found, uses generated placeholder sound
    bool loadSfx(SoundEffect sfx, const std::string& path);
    bool loadMusic(Music music, const std::string& path);

private:
    std::unordered_map<int, Mix_Chunk*> m_chunks;    // SoundEffect → chunk
    std::unordered_map<int, Mix_Music*> m_music;     // Music → music
    int  m_sfxVolume   = MIX_MAX_VOLUME;
    int  m_musicVolume = MIX_MAX_VOLUME * 2 / 3;
    bool m_initialized = false;

    Mix_Chunk* generateBeep(int frequency, int durationMs, int volume);
    void       loadAllPlaceholders();
};

}  // namespace LightsOut
