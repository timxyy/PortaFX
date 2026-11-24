#pragma once

#include "effect.h"

// Tremolo: a low-frequency oscillator (LFO) modulates the volume.
//
// The LFO is a sine wave.  Its output swings between 0 and 1, and
// we use it to scale the audio signal:
//
//     modulator = 1 − depth × ½ × (1 + sin(phase))
//
// When sin = −1  →  modulator = 1.0        (full volume)
// When sin = +1  →  modulator = 1 − depth  (quietest point)
//
// So `depth` controls how deep the volume dip is:
//   depth = 0    →  no effect (modulator is always 1)
//   depth = 0.5  →  volume swings between 1.0 and 0.5
//   depth = 1.0  →  volume swings between 1.0 and 0.0 (full silence at the dip)
//
// And `rate` is how many times per second the volume cycles (in Hz).
// Typical guitar tremolo: 4–8 Hz.
//
// Critical detail — phase persistence
// ------------------------------------
// The LFO phase must carry over from one block to the next.  If you
// reset it to 0 at the start of every block, two things go wrong:
//   1. You hear a click at every block boundary (discontinuity).
//   2. The effective tremolo rate depends on the block size, because
//      the sine only ever gets through its first (blockSize / sampleRate)
//      seconds before restarting.
//
// The roadmap's "done" test for Phase 1 is exactly this: changing
// block size from 64 to 512 must not change the output.

class Tremolo : public Effect {
public:
    // LFO speed in Hz (cycles per second).  4–8 Hz is typical.
    void setRate(float hz);

    // Modulation depth, 0.0 (no effect) to 1.0 (full silence at the dip).
    void setDepth(float d);

    // --- Effect interface ---
    void prepare(float sampleRate) override;
    void process(float* samples, size_t numSamples) override;
    void reset() override;

private:
    float sampleRate_ = 48000.0f;

    float rate_ = 5.0f;    // Hz
    float depth_ = 0.5f;   // 0–1

    // Phase of the LFO sine wave, in radians.
    // This is the state that MUST persist across blocks.
    float phase_ = 0.0f;

    // How much the phase advances per sample: 2π × rate / sampleRate.
    // Precomputed so process() doesn't divide every sample.
    float phaseInc_ = 0.0f;

    void updatePhaseInc();
};
