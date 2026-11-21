#include <algorithm>
#include <cstdio>
#include <exception>
#include <vector>

#include "wav.h"

// Run command: ./build/offline_fx testdata/sriff.wav testdata/out.wav
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: offline_fx <input.wav> <output.wav>\n");
        return 1;
    }

    WavFile wav;
    try {
        std::vector<uint8_t> file = read_file(argv[1]);
        wav = parse_wav(file);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    const WavInfo& info = wav.info;
    std::printf("format %u, %u ch, %u Hz, block_align %u, %u bits\n",
                info.audio_format, info.num_channels, info.sample_rate,
                info.block_align, info.bits_per_sample);
    std::printf("data at %zu, %zu frames = %.3f s\n",
                info.data_offset, info.num_frames(),
                double(info.num_frames()) / info.sample_rate);

    // Quick sanity check: range of each channel
    for (size_t c = 0; c < wav.samples.size(); ++c) {
        auto [lo, hi] = std::minmax_element(wav.samples[c].begin(), wav.samples[c].end());
        std::printf("ch%zu min %.5f max %.5f\n", c, *lo, *hi);
    }

    try {
        write_wav(argv[2], wav);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    std::printf("wrote %s\n", argv[2]);

    return 0;
}
