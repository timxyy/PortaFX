#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <cstring>
#include <cstdio>


void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    // pDevice: useless for this
    (void)pDevice;
    // pInput: samples from mic/pickup
    // pOutput: output sent to headphones/exciter
    
    // For mono float audio, 1 frame = 1 float:
    const float* in = static_cast<const float*>(pInput);
    float* out = static_cast<float*>(pOutput);

    // Clean passthrough: copy input straight to output
    std::memcpy(out, in, frameCount * sizeof(float));
}

int main() {
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);

    config.capture.format       = ma_format_f32; // float samples (-1.0 to 1.0)
    config.capture.channels     = 1;             // mono
    config.playback.format      = ma_format_f32;
    config.playback.channels    = 1;
    config.sampleRate           = 48000;
    config.periodSizeInFrames   = 128;          // buffer size: ~2.67 ms latency
    config.dataCallback         = dataCallback;  // pointer to your callback above

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        std::printf("Could not open audio device. Exiting program.");
        return 1;
    }

    // Start audio streaming
    
    ma_device_start(&device);

    std::printf("Running audio thread... Press Enter to quit.\n");
    std::getchar(); // Blocks the main thread while the audio thread runs in the background

    // Stop and clean up
    ma_device_uninit(&device);
}