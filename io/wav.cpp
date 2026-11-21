#include "wav.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

// reading
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
    for (int i = 0; i < 4; ++i) out |= (uint32_t)bytes[pos + i] << (i * 8);
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


// writing
static void write_U16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(v & 0xFF);
    out.push_back(v >> 8);
}

static void write_U32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back((v >> (i * 8)) & 0xFF);
}

static void write_ID(std::vector<uint8_t>& out, const char* id) {
    out.insert(out.end(), id, id + 4);
}

static int16_t float_to_int16(float x) {
    if (std::isnan(x)) return 0;
    float v = std::round(x * 32768.0f);
    return int16_t(std::clamp(v, -32768.0f, 32767.0f));
}

void write_wav(const std::string& path, const WavFile& wav) {
    const auto& ch = wav.samples;
    if (ch.empty()) throw std::runtime_error("No channels to write");
    size_t frames = ch[0].size();
    for (const auto& c : ch) if (c.size() != frames) throw std::runtime_error("Channels have different lengths");

    uint16_t num_channels = uint16_t(ch.size());
    uint16_t block_align = num_channels * 2;
    uint32_t sample_rate = wav.info.sample_rate;
    if (frames > (0xFFFFFFFFu - 36) / block_align) throw std::runtime_error("Too much audio for one WAV file");
    uint32_t data_size = uint32_t(frames * block_align);

    std::vector<uint8_t> out;
    out.reserve(44 + data_size);

    // RIFF header
    write_ID(out, "RIFF");
    write_U32(out, 36 + data_size);  // everything after this field: "WAVE" + fmt chunk (24) + data header (8) + samples
    write_ID(out, "WAVE");

    // fmt chunk
    write_ID(out, "fmt ");
    write_U32(out, 16);
    write_U16(out, 1);                          // integer PCM
    write_U16(out, num_channels);
    write_U32(out, sample_rate);
    write_U32(out, sample_rate * block_align);  // byte_rate
    write_U16(out, block_align);
    write_U16(out, 16);                         // bits per sample

    // data chunk: interleave the channels again, frame by frame
    write_ID(out, "data");
    write_U32(out, data_size);
    for (size_t f = 0; f < frames; ++f)
        for (const auto& c : ch)
            write_U16(out, uint16_t(float_to_int16(c[f])));
    // 16-bit samples always give an even data size, so no padding byte is needed

    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("File could not be created: " + path);
    file.write(reinterpret_cast<const char*>(out.data()), out.size());
    if (!file) throw std::runtime_error("File could not be written: " + path);
}
