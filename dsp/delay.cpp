#include "delay.h"

#include <algorithm>
#include <cmath>

// ---- public API ----

void Delay::setTimeMs(float ms) {
    timeMs_ = std::max(0.0f, ms);
    updateDelaySamples();
}

void Delay::setFeedback(float fb) {
    // Clamp to [0, 0.99] — feedback >= 1.0 means echoes grow forever.
    // Even 0.99 gives a long tail (each echo is 99% of the last).
    feedback_ = std::clamp(fb, 0.0f, 0.99f);
}

void Delay::setMix(float m) {
    mix_ = std::clamp(m, 0.0f, 1.0f);
}

// ---- Effect interface ----

void Delay::prepare(float sampleRate) {
    sampleRate_ = sampleRate;

    // Allocate the buffer for the maximum delay time.
    // +1 because we need room for the interpolation to look one sample ahead.
    size_t maxSamples = size_t(MAX_DELAY_MS * 0.001f * sampleRate) + 1;
    buffer_.assign(maxSamples, 0.0f);
    writePos_ = 0;

    updateDelaySamples();
}

void Delay::process(float* samples, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        float dry = samples[i];

        // Where to read from: writePos_ looking backwards by delaySamples_
        // We work in float so readInterpolated can handle the fractional part.
        float readPos = float(writePos_) - delaySamples_;
        if (readPos < 0.0f)
            readPos += float(buffer_.size());

        float wet = readInterpolated(readPos);

        // Write the input + fed-back output into the buffer.
        // This is what creates the repeating echoes: each time around,
        // the delayed signal (scaled by feedback) gets added to the new input.
        buffer_[writePos_] = dry + wet * feedback_;

        // Advance the write position, wrapping at the end of the buffer.
        writePos_ = (writePos_ + 1) % buffer_.size();

        // Mix dry and wet for the output.
        samples[i] = dry * (1.0f - mix_) + wet * mix_;
    }
}

void Delay::reset() {
    // Silence the buffer — clears any echo tail.
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writePos_ = 0;
}

// ---- private ----

void Delay::updateDelaySamples() {
    delaySamples_ = timeMs_ * 0.001f * sampleRate_;
    // Don't exceed the buffer.  Leave 1 sample headroom for interpolation.
    float maxDelay = float(buffer_.size()) - 1.0f;
    if (!buffer_.empty() && delaySamples_ > maxDelay)
        delaySamples_ = maxDelay;
}

float Delay::readInterpolated(float position) const {
    // Linear interpolation between two adjacent samples.
    //
    // position = 3.7 means: 70% of buffer[4] + 30% of buffer[3]
    //
    //   index0 = 3        (the sample just before)
    //   index1 = 4        (the sample just after)
    //   frac   = 0.7      (how far between them)
    //   result = buf[3] × 0.3 + buf[4] × 0.7

    size_t index0 = size_t(position);
    size_t index1 = (index0 + 1) % buffer_.size();
    float frac = position - float(index0);

    return buffer_[index0] * (1.0f - frac) + buffer_[index1] * frac;
}
