#include "gain.h"

#include <cmath>

// ---- helpers ----

// Convert decibels to a linear multiplier.
// The formula: linear = 10^(dB / 20)
//   -inf dB → 0.0,   0 dB → 1.0,   +6 dB → ~2.0
static float dbToLinear(float dB) {
    return std::pow(10.0f, dB / 20.0f);
}

// ---- public API ----

void Gain::setGainDb(float dB) {
    targetGain_ = dbToLinear(dB);
}

void Gain::setSmoothTimeMs(float ms) {
    smoothTimeMs_ = ms;
    updateAlpha();
}

// ---- Effect interface ----

void Gain::prepare(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAlpha();
}

void Gain::process(float* samples, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        // Slide the smoothed gain toward the target, one sample at a time.
        smoothedGain_ += alpha_ * (targetGain_ - smoothedGain_);

        samples[i] *= smoothedGain_;
    }
}

void Gain::reset() {
    // Snap the smoother to the target so the next block starts cleanly.
    smoothedGain_ = targetGain_;
}

// ---- private ----

void Gain::updateAlpha() {
    if (smoothTimeMs_ <= 0.0f) {
        // No smoothing: jump to the target instantly.
        alpha_ = 1.0f;
    } else {
        // One-pole coefficient from a time constant.
        // After `smoothTimeMs_` milliseconds, we'll be 63% of the way there.
        float timeSec = smoothTimeMs_ * 0.001f;
        alpha_ = 1.0f - std::exp(-1.0f / (timeSec * sampleRate_));
    }
}
