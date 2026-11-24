#include "tremolo.h"

#include <cmath>

// We need pi.  C++20 has std::numbers::pi, but a local constant is
// simpler and has no includes beyond <cmath>.
static constexpr float TWO_PI = 6.283185307179586f;

// ---- public API ----

void Tremolo::setRate(float hz) {
    rate_ = hz;
    updatePhaseInc();
}

void Tremolo::setDepth(float d) {
    // Clamp to [0, 1] so the modulator never goes negative.
    depth_ = d < 0.0f ? 0.0f : (d > 1.0f ? 1.0f : d);
}

// ---- Effect interface ----

void Tremolo::prepare(float sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseInc();
}

void Tremolo::process(float* samples, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        // Compute the LFO: a unipolar sine that swings from 0 to 1.
        //   sin(phase) is in [-1, +1]
        //   0.5 + 0.5 * sin(phase) is in [0, 1]
        float lfo = 0.5f + 0.5f * std::sin(phase_);

        // Turn the LFO into a volume multiplier:
        //   modulator = 1 − depth × lfo
        // When lfo = 0 → modulator = 1.0  (full volume)
        // When lfo = 1 → modulator = 1 − depth  (quietest)
        float modulator = 1.0f - depth_ * lfo;

        samples[i] *= modulator;

        // Advance the phase for the next sample.
        phase_ += phaseInc_;

        // Wrap to [0, 2π) so the float doesn't grow without bound.
        // After hours of running, an unwrapped phase would lose precision.
        if (phase_ >= TWO_PI)
            phase_ -= TWO_PI;
    }
}

void Tremolo::reset() {
    // Restart the LFO from the beginning.
    // Called between channels so stereo tremolo stays in sync.
    phase_ = 0.0f;
}

// ---- private ----

void Tremolo::updatePhaseInc() {
    // phase advances by (2π × rate / sampleRate) radians per sample.
    // Example: 5 Hz at 48 kHz → 0.000654 rad/sample → one full cycle
    //          every 9600 samples (200 ms), which is 5 times per second.
    phaseInc_ = TWO_PI * rate_ / sampleRate_;
}
