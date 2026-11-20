#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Everything we learn about a WAV file before decoding its samples.
// The first six fields are copied straight from the "fmt " chunk.
// The last two say where the samples live inside the file's bytes.
struct WavInfo {
    // --- from the "fmt " chunk
    uint16_t audio_format = 0;     // 1 = integer PCM, 3 = 32-bit float, 0xFFFE = extensible
    uint16_t num_channels = 0;     // 1 = mono, 2 = stereo
    uint32_t sample_rate = 0;      // frames per second, e.g. 44100 or 48000
    uint32_t byte_rate = 0;        // bytes per second = sample_rate * block_align
    uint16_t block_align = 0;      // bytes per frame (one sample for every channel)
    uint16_t bits_per_sample = 0;  // 16, 24, or 32

    // --- from the "data" chunk
    size_t data_offset = 0;        // index in the byte vector where the first sample starts
    uint32_t data_size = 0;        // number of sample bytes

    // How many frames (time steps) the file holds.
    size_t num_frames() const {
        return block_align == 0 ? 0 : data_size / block_align;
    }
};

// A decoded file: the format info plus the samples, one vector per channel.
struct WavFile {
    WavInfo info;
    std::vector<std::vector<float>> samples;  // samples[channel][frame], in [-1, 1]
};

std::vector<uint8_t> read_file(const std::string& path);
uint16_t read_U16(const std::vector<uint8_t>& bytes, size_t pos);
uint32_t read_U32(const std::vector<uint8_t>& bytes, size_t pos);

// Parses a whole WAV file held in memory. Throws std::runtime_error on bad or unsupported files.
WavFile parse_wav(const std::vector<uint8_t>& file);
