#include "wav.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

std::vector<uint8_t> read_file(const std::string& path) {
    // Open file as binary from the end
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("File could not be opened: " + path); // Catch io error

    // Since file was opened from end, this gets the size of the file
    std::streamsize size = file.tellg();
    // Put the tracker to the beginning
    file.seekg(0);

    std::vector<uint8_t> bytes(size); // Store the entire file into memory as chunks (bytes)
    file.read(reinterpret_cast<char*>(bytes.data()), size); // Reread the file as char*
    if (!file) throw std::runtime_error("File could not be read: " + path);
    return bytes;
}

uint16_t read_U16(const std::vector<uint8_t>& bytes, size_t pos) {
    return (uint16_t)bytes[pos + 1] << 8 | (uint16_t)bytes[pos];
}

uint32_t read_U32(const std::vector<uint8_t>& bytes, size_t pos) {
    uint32_t out = 0;
    for (int i = 0; i < 4; ++i) {
        out |= (uint32_t)bytes[pos + i] << (i * 8);
    }
    return out;
}

WavFile parse_wav(const std::vector<uint8_t>& file) {
    // RIFF header: "RIFF" <size> "WAVE" (12 bytes)
    if (file.size() < 12 || std::memcmp(&file[0], "RIFF", 4) != 0 || std::memcmp(&file[8], "WAVE", 4) != 0)
        throw std::runtime_error("Not a WAV file");

    // Chunk parsing starts at byte 12
    size_t pos = 12;
    size_t content;
    WavInfo info;
    bool found_fmt = false, found_data = false;

    while ((content = pos + 8) <= file.size()) {
        // Bytes 0-4: chunk ID
        // Bytes 4-8: remaining chunk size (ignoring padding)
        // 8-end: (actual chunk contents)

        uint32_t size = read_U32(file, pos + 4);
        // Subtract instead of adding so a huge size can't overflow on 32-bit chips
        if (size > file.size() - content) throw std::runtime_error("Chunk goes past end of file");

        if (std::memcmp(&file[pos], "fmt ", 4) == 0) {
            if (size < 16) throw std::runtime_error("Format chunk too small");
            info.audio_format = read_U16(file, content);
            info.num_channels = read_U16(file, content + 2);
            info.sample_rate = read_U32(file, content + 4);
            info.byte_rate = read_U32(file, content + 8);
            info.block_align = read_U16(file, content + 12);
            info.bits_per_sample = read_U16(file, content + 14);
            found_fmt = true;
        } else if (std::memcmp(&file[pos], "data", 4) == 0) {
            info.data_offset = content;
            info.data_size = size;
            found_data = true;
        }

        pos = content + size + size % 2;
    }

    if (!found_fmt || !found_data) throw std::runtime_error("Corrupt file: fmt or data chunk missing");
    if (info.audio_format != 1 || info.bits_per_sample != 16)
        throw std::runtime_error("Unsupported format: only 16-bit integer PCM for now");
    // Checking block_align first also guards the % below against dividing by zero
    if (info.num_channels == 0 || info.block_align != info.num_channels * 2)
        throw std::runtime_error("Corrupt file: block_align doesn't match channel count");
    if (info.data_size % info.block_align != 0)
        throw std::runtime_error("Corrupt file: data size isn't a whole number of frames");

    WavFile wav;
    wav.info = info;
    wav.samples.assign(info.num_channels, std::vector<float>(info.num_frames())); // samples[channel][frame]

    for (size_t frame = 0; frame < info.num_frames(); ++frame) {
        for (size_t channel = 0; channel < info.num_channels; ++channel) {
            int16_t sample = int16_t(read_U16(file, info.data_offset + frame * info.block_align + channel * 2));
            wav.samples[channel][frame] = sample / 32768.0f;
        }
    }

    return wav;
}
