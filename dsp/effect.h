#pragma once

#include <cstddef>

// Base class for every audio effect in the chain.
class Effect {
public:
    virtual ~Effect() = default;

    virtual void prepare(float sampleRate) = 0;
    virtual void process(float* samples, size_t numSamples) = 0;
    virtual void reset() = 0;
};
