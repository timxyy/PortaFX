#pragma once

#include "effect.h"

// A gain effect that scales the signal by a level set in decibels.
//
// Why dB?  Human hearing is logarithmic: −6 dB sounds like "half as loud,"
// +6 dB sounds like "twice as loud."  The conversion is:
//
//     linear_gain = 10^(dB / 20)
//
//   0 dB  →  1.0  (unity, no change)
//  −6 dB  →  0.5  (half amplitude)
// +12 dB  →  3.98 (almost 4x — will clip if the signal was already loud!)
//
// Parameter smoothing
// -------------------
// If you change the gain from 0 dB to −20 dB between two blocks, the
// volume jumps instantly — and that discontinuity sounds like a click
// ("zipper noise").  A one-pole smoother fixes this by sliding the gain
// toward its target a little bit each sample:
//
//     smoothed += alpha * (target − smoothed)
//
// alpha is a small number (0 < alpha < 1) that controls how fast the
// smoothing happens.  We compute it from a time constant in milliseconds:
//
//     alpha = 1 − exp(−1 / (timeMs * 0.001 * sampleRate))
//
// At the default 5 ms and 48 kHz, alpha ≈ 0.004, which means it takes
// roughly 5 ms to get 63% of the way to the new value, and about 20 ms
// to be inaudibly close.

class Gain : public Effect {
public:
    // Set the target gain in decibels.  The smoother will slide toward it.
    void setGainDb(float dB);

    // Set how long (ms) it takes for a gain change to fade in.
    // Default: 5 ms.  Set to 0 for instant (no smoothing).
    void setSmoothTimeMs(float ms);

    // --- Effect interface ---
    void prepare(float sampleRate) override;
    void process(float* samples, size_t numSamples) override;
    void reset() override;

private:
    float sampleRate_ = 48000.0f;

    // Target gain (in linear, not dB) and the smoother's current value
    float targetGain_ = 1.0f;    // 0 dB
    float smoothedGain_ = 1.0f;

    // Smoothing coefficient, recomputed whenever sample rate or time changes
    float smoothTimeMs_ = 5.0f;
    float alpha_ = 0.004f;

    void updateAlpha();
};
