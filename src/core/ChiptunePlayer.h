#pragma once
#include <SDL2/SDL_mixer.h>
#include <atomic>
#include <cstdint>

namespace LightsOut {

// Procedural chiptune synthesizer — Dance of the Sugar Plum Fairy
// with dubstep beats.  Hooks into SDL_mixer via Mix_HookMusic so it
// produces music without any audio files on disk.
//
// Thread safety: all mutable state is only touched from the SDL audio
// thread (inside the hook callback).  start()/stop() are safe to call
// from the main thread because they use an atomic flag and the SDK
// guarantees the audio thread is paused around Mix_HookMusic.
class ChiptunePlayer {
public:
    static ChiptunePlayer& get();

    // Register with SDL_mixer.  Call after Mix_OpenAudio succeeds.
    // sampleRate / channels must match what was passed to Mix_OpenAudio.
    void start(int sampleRate, int channels);

    // Unregister and silence.
    void stop();

    // 0–128  (128 = MIX_MAX_VOLUME)
    void setVolume(int vol);

    bool isPlaying() const { return m_active.load(); }

private:
    ChiptunePlayer() = default;

    // SDL_mixer hook — runs on the audio thread
    static void hookCallback(void* udata, Uint8* stream, int len);
    void synthesize(Sint16* out, int frames);

    // Called once per 16th-note step
    void onStep();

    // Per-sample generators — advance internal phase, return [-1,+1]
    float genMelody(double dt);
    float genBass(double dt);
    float genArp(double dt);
    float genKick(double dt);
    float genSnare(double dt);
    float genHihat(double dt);

    uint32_t xorshift();   // fast PRNG for noise

    // ── Song state (audio thread only) ────────────────────────────────────────
    double   m_songTime    = 0.0;   // elapsed seconds (for LFO reference)
    double   m_stepTime    = 0.0;   // seconds elapsed in current 16th note
    int      m_step        = 0;     // absolute 16th note counter

    // Melody voice (square wave)
    double   m_melPhase    = 0.0;
    double   m_melFreq     = 0.0;
    int      m_melNoteIdx  = 0;
    int      m_melNoteLen  = 0;     // 16ths remaining in current note

    // Bass voice (sawtooth + wobble LFO)
    double   m_bassPhase   = 0.0;
    double   m_bassFreq    = 0.0;
    int      m_bassNoteIdx = 0;
    int      m_bassNoteLen = 0;

    // Arpeggio voice (triangle wave, faster cycle)
    double   m_arpPhase    = 0.0;
    double   m_arpFreq     = 0.0;
    int      m_arpStep     = 0;

    // Drums
    float    m_kickEnv     = 0.0f;
    double   m_kickPhase   = 0.0;
    double   m_kickFreq    = 0.0;
    float    m_snareEnv    = 0.0f;
    float    m_hihatEnv    = 0.0f;

    uint32_t m_noiseSeed   = 0xC0FFEE17u;

    // Config
    int      m_sampleRate  = 44100;
    int      m_channels    = 2;
    double   m_bpm         = 140.0;
    int      m_volume      = 96;    // 0-128

    std::atomic<bool> m_active{false};
};

}  // namespace LightsOut
