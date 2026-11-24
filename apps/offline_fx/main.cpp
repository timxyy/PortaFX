#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <vector>

#include "chorus.h"
#include "delay.h"
#include "effect.h"
#include "gain.h"
#include "tremolo.h"
#include "wav.h"

// ---------------------------------------------------------------------------
// Usage:
//   ./offline_fx input.wav output.wav                      (passthrough)
//   ./offline_fx input.wav output.wav gain <dB>            (e.g. gain -6)
//   ./offline_fx input.wav output.wav tremolo <Hz> <depth> (e.g. tremolo 5 0.8)
// ---------------------------------------------------------------------------

static void printUsage() {
    std::fprintf(stderr,
        "usage: offline_fx <input.wav> <output.wav> [effect [args...]]\n"
        "\n"
        "effects:\n"
        "  gain <dB>              scale the signal (e.g. -6, +12)\n"
        "  tremolo <Hz> <depth>   sine LFO volume wobble (e.g. 5 0.8)\n"
        "  delay <ms> <fb> <mix>  echo with feedback (e.g. 300 0.5 0.5)\n"
        "  chorus <Hz> <depth_ms> <mix>  modulated delay (e.g. 1.5 3 0.5)\n"
        "\n"
        "no effect = passthrough (null test)\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    // Parse input
    WavFile wav;
    try {
        std::vector<uint8_t> file = read_file(argv[1]);
        wav = parse_wav(file);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    const WavInfo& info = wav.info;
    std::printf("format %u, %u ch, %u Hz, %u bits, %zu frames (%.3f s)\n",
                info.audio_format, info.num_channels, info.sample_rate,
                info.bits_per_sample, info.num_frames(),
                double(info.num_frames()) / info.sample_rate);

    // Pointer to effect. Makes effect changeable.
    std::unique_ptr<Effect> effect;

    if (argc >= 4) {
        if (std::strcmp(argv[3], "gain") == 0) {
            if (argc < 5) {
                std::fprintf(stderr, "gain needs a dB value (e.g. gain -6)\n");
                return 1;
            }
            auto g = std::make_unique<Gain>();
            g->setGainDb(std::atof(argv[4]));
            std::printf("effect: gain %.1f dB\n", std::atof(argv[4]));
            effect = std::move(g);

        } else if (std::strcmp(argv[3], "tremolo") == 0) {
            if (argc < 6) {
                std::fprintf(stderr, "tremolo needs rate and depth (e.g. tremolo 5 0.8)\n");
                return 1;
            }
            auto t = std::make_unique<Tremolo>();
            float rate = std::atof(argv[4]);
            float depth = std::atof(argv[5]);
            t->setRate(rate);
            t->setDepth(depth);
            std::printf("effect: tremolo %.1f Hz, depth %.2f\n", rate, depth);
            effect = std::move(t);

        } else if (std::strcmp(argv[3], "delay") == 0) {
            if (argc < 7) {
                std::fprintf(stderr, "delay needs time_ms, feedback, mix (e.g. delay 300 0.5 0.5)\n");
                return 1;
            }
            auto d = std::make_unique<Delay>();
            float timeMs = std::atof(argv[4]);
            float fb = std::atof(argv[5]);
            float mix = std::atof(argv[6]);
            d->setTimeMs(timeMs);
            d->setFeedback(fb);
            d->setMix(mix);
            std::printf("effect: delay %.1f ms, feedback %.2f, mix %.2f\n", timeMs, fb, mix);
            effect = std::move(d);

        } else if (std::strcmp(argv[3], "chorus") == 0) {
            if (argc < 7) {
                std::fprintf(stderr, "chorus needs rate_hz, depth_ms, mix (e.g. chorus 1.5 3 0.5)\n");
                return 1;
            }
            auto c = std::make_unique<Chorus>();
            float rate = std::atof(argv[4]);
            float depthMs = std::atof(argv[5]);
            float mix = std::atof(argv[6]);
            c->setRate(rate);
            c->setDepthMs(depthMs);
            c->setMix(mix);
            std::printf("effect: chorus %.1f Hz, depth %.1f ms, mix %.2f\n", rate, depthMs, mix);
            effect = std::move(c);

        } else {
            std::fprintf(stderr, "unknown effect: %s\n", argv[3]);
            printUsage();
            return 1;
        }
    }

    // Block processing which prepares to transfer to live audio processing.
    if (effect) {
        const size_t blockSize = 128;  // try changing this to 64, 256, 512
        effect->prepare(float(info.sample_rate));

        for (auto& channel : wav.samples) {
            effect->reset();  // sync state across channels

            for (size_t i = 0; i < channel.size(); i += blockSize) {
                size_t n = std::min(blockSize, channel.size() - i);
                effect->process(channel.data() + i, n);
            }
        }
    }

    // Create output file
    try {
        write_wav(argv[2], wav);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    std::printf("wrote %s\n", argv[2]);

    return 0;
}
