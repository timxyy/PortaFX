#pragma once

#include "effect.h"

#include <vector>

// Chorus: a short, LFO-modulated delay that makes one instrument
// sound like several playing in slight unison.
//
// How it works:
//   1. The input is written into a short delay line (~10–25 ms).
//   2. A sine LFO slowly sweeps the read position back and forth
//      within that range.
//   3. The moving read position causes slight pitch shifts — when the
//      delay gets shorter the pitch rises, when it gets longer the
//      pitch drops.  This "detuning" is what creates the chorus effect.
//   4. The delayed (wet) signal is mixed with the original (dry).
//
// Because the read position moves continuously between samples,
// we reuse the same linear interpolation from the Delay class.
//
// Controls:
//   depth  — how far (in ms) the LFO sweeps the delay time.
//            The delay swings from (center − depth) to (center + depth).
//   rate   — LFO speed in Hz.  Typical chorus: 0.5–3 Hz.
//   mix    — dry/wet blend.  0.5 is a good starting point.
//
// A flanger is the same algorithm with a much shorter center delay
// (~1–5 ms instead of ~15 ms) and optional feedback.  You get one
// for free by just dialing the numbers down.

class Chorus : public Effect {
public:
    // LFO speed in Hz.  0.5–3 Hz is typical.
    void setRate(float hz);

    // How far the delay sweeps, in milliseconds.
    // The total delay range is centerMs ± depth.
    void setDepthMs(float ms);

    // Dry/wet blend.  0.0 = dry only, 1.0 = wet only.
    void setMix(float m);

    // --- Effect interface ---
    void prepare(float sampleRate) override;
    void process(float* samples, size_t numSamples) override;
    void reset() override;

private:
    float sampleRate_ = 48000.0f;

    // LFO
    float rate_ = 1.5f;     // Hz
    float depthMs_ = 3.0f;  // ms of sweep
    float mix_ = 0.5f;

    float phase_ = 0.0f;
    float phaseInc_ = 0.0f;

    // The center of the delay sweep, in ms.  Fixed at a typical chorus value.
    static constexpr float CENTER_MS = 15.0f;

    // Buffer large enough for the longest possible delay
    static constexpr float MAX_DEPTH_MS = 15.0f;  // generous headroom
    std::vector<float> buffer_;
    size_t writePos_ = 0;

    void updatePhaseInc();

    // Same linear interpolation as Delay
    float readInterpolated(float position) const;
};
