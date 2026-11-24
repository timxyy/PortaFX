#pragma once

#include "effect.h"

#include <vector>

// Delay: echoes of the input signal, using a circular buffer.
//
// A circular (ring) buffer is a fixed-size array with a write position
// that wraps around to the beginning when it reaches the end.  To read
// a delayed sample, you look backwards from the write position:
//
//     write index:  0  1  2  3  4  5  6  7  0  1  2  ...
//                   ↑──────────────────────↑
//                   wraps around here
//
//     read index = write index − delaySamples
//     (if negative, add the buffer length to wrap)
//
// Controls:
//   time     — delay time in milliseconds (converted to samples internally)
//   feedback — how much of the output is fed back into the buffer (0–1)
//              0.0 = single echo, 0.5 = decaying repeats, ≥1.0 = infinite/growing
//   mix      — blend between dry (original) and wet (delayed) signal
//              0.0 = fully dry, 1.0 = fully wet
//
// Wet-only mode (mix = 1.0) outputs just the delayed signal — this is
// what you'll eventually feed into the exciter on the guitar body.
//
// Fractional delay
// ----------------
// When the delay time doesn't land on an exact sample, we need a value
// "between" two samples.  Linear interpolation gets us there:
//
//     result = sample[floor] × (1 − frac) + sample[ceil] × frac
//
// This matters for chorus, where the delay time moves continuously.
// Without interpolation you'd hear stepping artifacts as the read
// position jumps between integer samples.

class Delay : public Effect {
public:
    // Delay time in milliseconds.  Converted to samples using the sample rate.
    void setTimeMs(float ms);

    // How much of the delayed output feeds back into the delay line (0–1).
    // Values >= 1.0 are clamped to 0.99 to prevent runaway feedback.
    void setFeedback(float fb);

    // Dry/wet mix.  0.0 = bypass, 1.0 = wet only.
    void setMix(float m);

    // --- Effect interface ---
    void prepare(float sampleRate) override;
    void process(float* samples, size_t numSamples) override;
    void reset() override;

private:
    float sampleRate_ = 48000.0f;

    // User-facing parameters (in human units)
    float timeMs_ = 300.0f;
    float feedback_ = 0.5f;
    float mix_ = 0.5f;

    // The circular buffer and its write position
    std::vector<float> buffer_;
    size_t writePos_ = 0;

    // Delay time expressed in samples (may be fractional)
    float delaySamples_ = 0.0f;

    // Maximum delay in samples (sets the buffer size)
    static constexpr float MAX_DELAY_MS = 2000.0f;

    void updateDelaySamples();

    // Read from the buffer at a fractional position using linear interpolation
    float readInterpolated(float position) const;
};
