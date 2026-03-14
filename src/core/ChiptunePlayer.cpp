#include "core/ChiptunePlayer.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

namespace LightsOut {

// ─── Helpers ──────────────────────────────────────────────────────────────────
static constexpr double PI2 = 2.0 * std::numbers::pi;

static double midiFreq(int midi) {
    // 0 = rest
    return (midi > 0) ? 440.0 * std::pow(2.0, (midi - 69) / 12.0) : 0.0;
}
static float squareWave(float phase, float duty = 0.5f) {
    return (std::fmod(phase, 1.0f) < duty) ? 1.0f : -1.0f;
}
static float sawWave(float phase) {
    return 2.0f * std::fmod(phase, 1.0f) - 1.0f;
}
static float triWave(float phase) {
    float p = std::fmod(phase, 1.0f);
    return (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
}

// ─── Song data ────────────────────────────────────────────────────────────────
// MIDI note numbers; 0 = rest.  Duration in 16th notes at 140 BPM.
struct Note { int midi; int sixteenths; };

// ── Dance of the Sugar Plum Fairy (E minor, Tchaikovsky) ─────────────────────
// Drums run at 140 BPM.  Melody runs at HALF that rate (dubstep half-time):
// all durations doubled so q=8, e=4, h=16, w=32 sixteenths → 70 BPM felt tempo.
static const Note MELODY[] = {
    // ── Phrase A ─────────────────────────────────────────────────────────────
    // bar 1: E5(q) F#5(e) G#5(e) A5(q) B5(q)  — ascending E major pentatonic
    {76,8},{78,4},{80,4},{81,8},{83,8},
    // bar 2: C#6(h) rest(h)
    {85,16},{0,16},
    // bar 3: repeat bar 1
    {76,8},{78,4},{80,4},{81,8},{83,8},
    // bar 4: A5(h) rest(h)
    {81,16},{0,16},
    // bar 5: D6(q) C#6(e) B5(e) A5(q) G#5(q)
    {86,8},{85,4},{83,4},{81,8},{80,8},
    // bar 6: F#5(h) rest(h)
    {78,16},{0,16},
    // bar 7: A5(q) G#5(e) F#5(e) E5(q) Eb5(q)
    {81,8},{80,4},{78,4},{76,8},{75,8},
    // bar 8: E5(w)
    {76,32},

    // ── Phrase B: higher-register variation ───────────────────────────────────
    // bar 9:  B5(q) C#6(e) E6(e) F#6(q) E6(q)
    {83,8},{85,4},{88,4},{90,8},{88,8},
    // bar 10: E6(h) rest(h)
    {88,16},{0,16},
    // bar 11: B5(q) C#6(e) E6(e) F#6(q) E6(q)
    {83,8},{85,4},{88,4},{90,8},{88,8},
    // bar 12: C#6(h) rest(h)
    {85,16},{0,16},
    // bar 13: D6(q) C#6(e) B5(e) A5(q) G#5(q)
    {86,8},{85,4},{83,4},{81,8},{80,8},
    // bar 14: F#5(h) rest(h)
    {78,16},{0,16},
    // bar 15: A5(q) G#5(e) F#5(e) E5(q) D5(q)
    {81,8},{80,4},{78,4},{76,8},{74,8},
    // bar 16: E5(w) — loop
    {76,32},
};
static constexpr int MELODY_N = static_cast<int>(std::size(MELODY));

// ── Bass: E minor root/fifth, held across 4 bars each ────────────────────────
static const Note BASS[] = {
    {40,64},{45,64},{40,64},{47,64},   // phrase A: E2 A2 E2 B2
    {40,64},{45,64},{40,64},{47,64},   // phrase B
};
static constexpr int BASS_N = static_cast<int>(std::size(BASS));

// ── Arp notes: E minor pentatonic (cycle every 2 steps) ──────────────────────
static const int ARP[] = {52, 56, 59, 61, 64, 61, 59, 56};  // E3 G#3 B3 C#4 E4 ...
static constexpr int ARP_N = static_cast<int>(std::size(ARP));

// ── Drum pattern (16 steps, repeats every bar) ───────────────────────────────
// Bits: 0=kick  1=snare  2=hihat
// Dubstep half-time: kick on 1 & syncopated, snare on beat 3 (step 8)
//        0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
static const uint8_t DRUMS[16] = {
    0x05, 0x00, 0x04, 0x01, 0x04, 0x00, 0x05, 0x00,  //  K+H  .    H    k    H    .   K+H   .
    0x06, 0x00, 0x04, 0x01, 0x04, 0x00, 0x05, 0x04,  //  K+S+H .   H    k    H    .   K+H   H
};

// ─── Singleton ────────────────────────────────────────────────────────────────
ChiptunePlayer& ChiptunePlayer::get() {
    static ChiptunePlayer inst;
    return inst;
}

// ─── Public API ───────────────────────────────────────────────────────────────
void ChiptunePlayer::start(int sampleRate, int channels) {
    // Reset state (safe: called before hooking, audio thread not yet active)
    m_sampleRate  = sampleRate;
    m_channels    = channels;
    m_songTime    = 0.0;
    m_stepTime    = 0.0;
    m_step        = 0;

    m_melPhase    = m_bassPhase = m_arpPhase = 0.0;
    m_melNoteIdx  = 0;
    m_melNoteLen  = MELODY[0].sixteenths;
    m_melFreq     = midiFreq(MELODY[0].midi);
    m_bassNoteIdx = 0;
    m_bassNoteLen = BASS[0].sixteenths;
    m_bassFreq    = midiFreq(BASS[0].midi);
    m_arpStep     = 0;
    m_arpFreq     = midiFreq(ARP[0]);
    m_arpPhase    = 0.0;

    m_kickEnv     = 0.0f;
    m_kickPhase   = 0.0;
    m_kickFreq    = 0.0;
    m_snareEnv    = 0.0f;
    m_hihatEnv    = 0.0f;
    m_noiseSeed   = 0xC0FFEE17u;

    m_active.store(true);
    Mix_HookMusic(hookCallback, this);
}

void ChiptunePlayer::stop() {
    Mix_HookMusic(nullptr, nullptr);
    m_active.store(false);
}

void ChiptunePlayer::setVolume(int vol) {
    m_volume = std::clamp(vol, 0, 128);
}

// ─── SDL_mixer callback (audio thread) ───────────────────────────────────────
void ChiptunePlayer::hookCallback(void* udata, Uint8* stream, int len) {
    auto* self = static_cast<ChiptunePlayer*>(udata);
    // len bytes = len / (channels * sizeof(Sint16)) stereo frames
    int frames = len / (self->m_channels * static_cast<int>(sizeof(Sint16)));
    SDL_memset(stream, 0, len);
    if (self->m_active.load())
        self->synthesize(reinterpret_cast<Sint16*>(stream), frames);
}

// ─── Sequencer step ──────────────────────────────────────────────────────────
void ChiptunePlayer::onStep() {
    ++m_step;

    // Advance melody
    --m_melNoteLen;
    if (m_melNoteLen <= 0) {
        m_melNoteIdx = (m_melNoteIdx + 1) % MELODY_N;
        m_melNoteLen = MELODY[m_melNoteIdx].sixteenths;
        double f = midiFreq(MELODY[m_melNoteIdx].midi);
        if (f != m_melFreq) { m_melFreq = f; m_melPhase = 0.0; }
    }

    // Advance bass
    --m_bassNoteLen;
    if (m_bassNoteLen <= 0) {
        m_bassNoteIdx = (m_bassNoteIdx + 1) % BASS_N;
        m_bassNoteLen = BASS[m_bassNoteIdx].sixteenths;
        double f = midiFreq(BASS[m_bassNoteIdx].midi);
        if (f != m_bassFreq) { m_bassFreq = f; m_bassPhase = 0.0; }
    }

    // Advance arpeggio every 2 steps
    if (m_step % 2 == 0) {
        m_arpStep = (m_arpStep + 1) % ARP_N;
        m_arpFreq = midiFreq(ARP[m_arpStep]);
        m_arpPhase = 0.0;
    }

    // Trigger drums
    uint8_t pat = DRUMS[m_step % 16];
    if (pat & 0x1) { m_kickEnv = 1.0f; m_kickFreq = 160.0; m_kickPhase = 0.0; }
    if (pat & 0x2) { m_snareEnv = 1.0f; }
    if (pat & 0x4) { m_hihatEnv = 1.0f; }
}

// ─── Per-sample generators ────────────────────────────────────────────────────
uint32_t ChiptunePlayer::xorshift() {
    m_noiseSeed ^= m_noiseSeed << 13;
    m_noiseSeed ^= m_noiseSeed >> 17;
    m_noiseSeed ^= m_noiseSeed << 5;
    return m_noiseSeed;
}

float ChiptunePlayer::genMelody(double dt) {
    if (m_melFreq <= 0.0) return 0.0f;
    // Triangle wave: softer harmonics, celesta-like clarity
    float s = triWave(static_cast<float>(m_melPhase));
    m_melPhase += m_melFreq * dt;
    return s;
}

float ChiptunePlayer::genBass(double dt) {
    if (m_bassFreq <= 0.0) return 0.0f;
    // Sawtooth with a 4 Hz wobble LFO (dubstep growl)
    float lfo = 0.5f + 0.5f * std::sin(static_cast<float>(PI2 * 4.0 * m_songTime));
    lfo = lfo * lfo;  // square it for a sharper wobble shape
    float s = sawWave(static_cast<float>(m_bassPhase)) * lfo;
    m_bassPhase += m_bassFreq * dt;
    return s;
}

float ChiptunePlayer::genArp(double dt) {
    if (m_arpFreq <= 0.0 || m_melFreq <= 0.0) return 0.0f;
    float s = triWave(static_cast<float>(m_arpPhase));
    m_arpPhase += m_arpFreq * dt;
    return s;
}

float ChiptunePlayer::genKick(double dt) {
    if (m_kickEnv <= 0.0f) return 0.0f;
    float s = std::sin(static_cast<float>(PI2 * m_kickPhase));
    s *= m_kickEnv;
    // Pitch drops fast (click + thump)
    m_kickFreq  = std::max(m_kickFreq - 350.0 * dt, 45.0);
    m_kickPhase += m_kickFreq * dt;
    // Exponential decay (~0.25 s)
    m_kickEnv  *= std::pow(0.001f, static_cast<float>(dt) / 0.25f);
    if (m_kickEnv < 0.001f) m_kickEnv = 0.0f;
    return s;
}

float ChiptunePlayer::genSnare(double dt) {
    if (m_snareEnv <= 0.0f) return 0.0f;
    float noise  = static_cast<float>(static_cast<int32_t>(xorshift())) / 2147483648.0f;
    float tone   = std::sin(static_cast<float>(PI2 * 180.0 * m_songTime));
    float s      = (noise * 0.65f + tone * 0.35f) * m_snareEnv;
    // Decay ~0.18 s
    m_snareEnv *= std::pow(0.001f, static_cast<float>(dt) / 0.18f);
    if (m_snareEnv < 0.001f) m_snareEnv = 0.0f;
    return s;
}

float ChiptunePlayer::genHihat(double dt) {
    if (m_hihatEnv <= 0.0f) return 0.0f;
    float noise = static_cast<float>(static_cast<int32_t>(xorshift())) / 2147483648.0f;
    float s     = noise * m_hihatEnv;
    // Very short decay ~0.04 s
    m_hihatEnv *= std::pow(0.001f, static_cast<float>(dt) / 0.04f);
    if (m_hihatEnv < 0.001f) m_hihatEnv = 0.0f;
    return s;
}

// ─── Main synthesis loop ──────────────────────────────────────────────────────
void ChiptunePlayer::synthesize(Sint16* out, int frames) {
    const double stepSec = 60.0 / (m_bpm * 4.0);  // seconds per 16th note
    const double dt      = 1.0 / m_sampleRate;

    // Melody is dominant; everything else supports it
    const float vol      = static_cast<float>(m_volume) / 128.0f;
    const float vMel     = 0.55f * vol;
    const float vBass    = 0.18f * vol;
    const float vArp     = 0.06f * vol;
    const float vKick    = 0.30f * vol;
    const float vSnare   = 0.18f * vol;
    const float vHihat   = 0.07f * vol;

    for (int i = 0; i < frames; ++i) {
        // ── Step sequencer ────────────────────────────────────────────────────
        m_stepTime += dt;
        while (m_stepTime >= stepSec) {
            m_stepTime -= stepSec;
            onStep();
        }

        // ── Generate voices ───────────────────────────────────────────────────
        float mel   = genMelody(dt) * vMel;
        float bass  = genBass(dt)   * vBass;
        float arp   = genArp(dt)    * vArp;
        float kick  = genKick(dt)   * vKick;
        float snare = genSnare(dt)  * vSnare;
        float hihat = genHihat(dt)  * vHihat;

        float mix = mel + bass + arp + kick + snare + hihat;

        // Soft clip via tanh to avoid harsh digital distortion
        mix = std::tanh(mix * 1.2f);

        Sint16 s = static_cast<Sint16>(mix * 32700.0f);

        // Write to all channels
        for (int ch = 0; ch < m_channels; ++ch)
            out[i * m_channels + ch] = s;

        m_songTime += dt;
    }
}

}  // namespace LightsOut
