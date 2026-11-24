#include "chorus.h"

#include <algorithm>
#include <cmath>

static constexpr float TWO_PI = 6.283185307179586f;

// ---- public API ----

void Chorus::setRate(float hz) {
    rate_ = hz;
    updatePhaseInc();
}

void Chorus::setDepthMs(float ms) {
    depthMs_ = std::max(0.0f, ms);
}

void Chorus::setMix(float m) {
    mix_ = std::clamp(m, 0.0f, 1.0f);
}

// ---- Effect interface ----

void Chorus::prepare(float sampleRate) {
    sampleRate_ = sampleRate;

    // Buffer must hold the longest possible delay:
    //   center + max depth + 1 for interpolation headroom
    float maxDelayMs = CENTER_MS + MAX_DEPTH_MS;
    size_t maxSamples = size_t(maxDelayMs * 0.001f * sampleRate) + 2;
    buffer_.assign(maxSamples, 0.0f);
    writePos_ = 0;

    updatePhaseInc();
}

void Chorus::process(float* samples, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        float dry = samples[i];

        // Write the dry input into the delay line
        buffer_[writePos_] = dry;

        // The LFO sweeps the delay time around the center.
        // sin(phase) is in [-1, 1], so the delay swings by ±depthMs_.
        float delayMs = CENTER_MS + depthMs_ * std::sin(phase_);
        float delaySamples = delayMs * 0.001f * sampleRate_;

        // Calculate the read position (looking backwards from writePos)
        float readPos = float(writePos_) - delaySamples;
        if (readPos < 0.0f)
            readPos += float(buffer_.size());

        float wet = readInterpolated(readPos);

        // Mix
        samples[i] = dry * (1.0f - mix_) + wet * mix_;

        // Advance write position
        writePos_ = (writePos_ + 1) % buffer_.size();

        // Advance LFO phase
        phase_ += phaseInc_;
        if (phase_ >= TWO_PI)
            phase_ -= TWO_PI;
    }
}

void Chorus::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writePos_ = 0;
    phase_ = 0.0f;
}

// ---- private ----

void Chorus::updatePhaseInc() {
    phaseInc_ = TWO_PI * rate_ / sampleRate_;
}

float Chorus::readInterpolated(float position) const {
    size_t index0 = size_t(position);
    size_t index1 = (index0 + 1) % buffer_.size();
    float frac = position - float(index0);
    return buffer_[index0] * (1.0f - frac) + buffer_[index1] * frac;
}
