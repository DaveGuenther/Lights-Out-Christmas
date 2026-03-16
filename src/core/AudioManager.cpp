#include "core/AudioManager.h"
#include "core/ChiptunePlayer.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace LightsOut {

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::init() {
    int mixFlags = MIX_INIT_OGG | MIX_INIT_MP3;
    int mixLoaded = Mix_Init(mixFlags);
    if (!(mixLoaded & MIX_INIT_OGG))
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "SDL_mixer OGG support unavailable: %s", Mix_GetError());
    if (!(mixLoaded & MIX_INIT_MP3))
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "SDL_mixer MP3 support unavailable: %s", Mix_GetError());
    if (Mix_OpenAudio(AUDIO_FREQUENCY, MIX_DEFAULT_FORMAT, AUDIO_CHANNELS, AUDIO_CHUNK_SIZE) < 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "SDL_mixer init failed: %s — audio disabled", Mix_GetError());
        return false;  // non-fatal
    }
    Mix_AllocateChannels(AUDIO_MIX_CHANNELS);
    m_initialized = true;
    loadAllPlaceholders();
    return true;
}

void AudioManager::shutdown() {
    if (!m_initialized) return;
    ChiptunePlayer::get().stop();
    for (auto& [id, chunk] : m_chunks) {
        if (chunk) Mix_FreeChunk(chunk);
    }
    m_chunks.clear();
    for (auto& [id, music] : m_music) {
        if (music) Mix_FreeMusic(music);
    }
    m_music.clear();
    Mix_CloseAudio();
    m_initialized = false;
}

void AudioManager::playSfx(SoundEffect sfx, int volume) {
    if (!m_initialized) return;
    auto it = m_chunks.find(static_cast<int>(sfx));
    if (it == m_chunks.end() || !it->second) return;
    Mix_VolumeChunk(it->second, volume * m_sfxVolume / MIX_MAX_VOLUME);
    Mix_PlayChannel(-1, it->second, 0);
}

void AudioManager::playMusic(Music music, bool /*loop*/) {
    if (!m_initialized) return;
    // Use the procedural chiptune player for all background music.
    // If a real OGG file was loaded for this track, prefer it; otherwise fall
    // back to the chiptune generator so there is always something playing.
    ChiptunePlayer& cp = ChiptunePlayer::get();
    auto it = m_music.find(static_cast<int>(music));
    if (it != m_music.end() && it->second) {
        // A real audio file was loaded — stop chiptune and play it
        cp.stop();
        Mix_PlayMusic(it->second, -1);
        Mix_VolumeMusic(m_musicVolume);
    } else {
        // No audio file — start the procedural chiptune engine
        Mix_HaltMusic();   // stop any previous SDL_mixer music
        cp.setVolume(m_musicVolume);
        cp.start(AUDIO_FREQUENCY, AUDIO_CHANNELS);
    }
}

void AudioManager::stopMusic() {
    if (!m_initialized) return;
    ChiptunePlayer::get().stop();
    Mix_HaltMusic();
}
void AudioManager::pauseMusic() {
    if (!m_initialized) return;
    ChiptunePlayer::get().setVolume(0);
    Mix_PauseMusic();
}
void AudioManager::resumeMusic() {
    if (!m_initialized) return;
    ChiptunePlayer::get().setVolume(m_musicVolume);
    Mix_ResumeMusic();
}

void AudioManager::setMusicIntensity(float intensity) {
    if (!m_initialized) return;
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    int vol = static_cast<int>(m_musicVolume * (0.4f + 0.6f * intensity));
    Mix_VolumeMusic(vol);
    if (ChiptunePlayer::get().isPlaying())
        ChiptunePlayer::get().setVolume(vol);
}

void AudioManager::setMasterVolume(int vol) {
    setSfxVolume(vol);
    setMusicVolume(vol);
}

void AudioManager::setSfxVolume(int vol) {
    m_sfxVolume = std::max(0, std::min(MIX_MAX_VOLUME, vol));
}

void AudioManager::setMusicVolume(int vol) {
    m_musicVolume = std::clamp(vol, 0, MIX_MAX_VOLUME);
    if (!m_initialized) return;
    Mix_VolumeMusic(m_musicVolume);
    if (ChiptunePlayer::get().isPlaying())
        ChiptunePlayer::get().setVolume(m_musicVolume);
}

bool AudioManager::isMusicPlaying() const {
    return m_initialized &&
           (Mix_PlayingMusic() != 0 || ChiptunePlayer::get().isPlaying());
}

bool AudioManager::loadSfx(SoundEffect sfx, const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not load SFX '%s': %s", path.c_str(), Mix_GetError());
        return false;
    }
    int key = static_cast<int>(sfx);
    if (m_chunks.count(key) && m_chunks[key]) Mix_FreeChunk(m_chunks[key]);
    m_chunks[key] = chunk;
    return true;
}

bool AudioManager::loadMusic(Music music, const std::string& path) {
    Mix_Music* m = Mix_LoadMUS(path.c_str());
    if (!m) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not load music '%s': %s", path.c_str(), Mix_GetError());
        return false;
    }
    int key = static_cast<int>(music);
    if (m_music.count(key) && m_music[key]) Mix_FreeMusic(m_music[key]);
    m_music[key] = m;
    return true;
}

Mix_Chunk* AudioManager::generateBeep(int frequency, int durationMs, int volume) {
    const int sampleRate = AUDIO_FREQUENCY;
    const int numSamples = sampleRate * durationMs / 1000;
    const int bufSize    = numSamples * 2;  // 16-bit = 2 bytes per sample

    Uint8* buf = static_cast<Uint8*>(SDL_malloc(bufSize));
    if (!buf) return nullptr;

    Sint16* samples = reinterpret_cast<Sint16*>(buf);
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        // Sine wave with simple envelope (attack + decay)
        float env = 1.0f;
        float attack = 0.01f;
        float decay  = static_cast<float>(durationMs) / 1000.0f * 0.3f;
        if (t < attack) env = t / attack;
        else {
            float elapsed = t - attack;
            float total   = static_cast<float>(durationMs) / 1000.0f - attack;
            env = 1.0f - std::min(1.0f, elapsed / total);
        }
        float wave = std::sin(2.0f * std::numbers::pi_v<float> * frequency * t);
        samples[i] = static_cast<Sint16>(wave * env * volume * 256);
    }

    Mix_Chunk* chunk = static_cast<Mix_Chunk*>(SDL_malloc(sizeof(Mix_Chunk)));
    if (!chunk) { SDL_free(buf); return nullptr; }
    chunk->abuf     = buf;
    chunk->alen     = bufSize;
    chunk->allocated = 1;
    chunk->volume   = MIX_MAX_VOLUME;
    return chunk;
}

void AudioManager::loadAllPlaceholders() {
    struct SfxDef { SoundEffect sfx; int freq; int ms; };
    static const SfxDef defs[] = {
        { SoundEffect::LightPop,       800, 80  },
        { SoundEffect::LightCascade,   600, 200 },
        { SoundEffect::Chomp,          300, 100 },
        { SoundEffect::HomeownerAlert, 400, 300 },
        { SoundEffect::DogBark,        200, 150 },
        { SoundEffect::PowerUpCollect, 1000,150 },
        { SoundEffect::PowerUpEnd,     500, 100 },
        { SoundEffect::LaneJump,       700,  60 },
        { SoundEffect::Spark,          1200, 50 },
        { SoundEffect::StageComplete,  900, 400 },
        { SoundEffect::GameOver,       250, 500 },
        { SoundEffect::ComboChime,     1100,120 },
    };
    for (const auto& d : defs) {
        int key = static_cast<int>(d.sfx);
        m_chunks[key] = generateBeep(d.freq, d.ms, MIX_MAX_VOLUME / 2);
    }
}

}  // namespace LightsOut
