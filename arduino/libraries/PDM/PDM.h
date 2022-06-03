#ifndef PDM_H_
#define PDM_H_

#include <memory>
#include <optional>

#include "Arduino.h"
#include "libs/audio/audio_driver.h"
#include "libs/audio/audio_service.h"

namespace {
// from micro_model_settings.h

// Only 16kHz and 48kHz are supported
constexpr int kAudioSampleFrequency = 16000;

constexpr int kSamplesPerMs = kAudioSampleFrequency / 1000;

constexpr int kNumDmaBuffers = 10;
constexpr int kDmaBufferSizeMs = 100;
constexpr int kDmaBufferSize = kDmaBufferSizeMs * kSamplesPerMs;
coral::micro::AudioDriverBuffers<kNumDmaBuffers,
                                 kNumDmaBuffers * kDmaBufferSize>
    g_audio_buffers;

constexpr int kAudioBufferSizeMs = 1000;
constexpr int kAudioBufferSize = kAudioBufferSizeMs * kSamplesPerMs;
int16_t g_audio_buffer[kAudioBufferSize] __attribute__((aligned(16)));

constexpr int kDropFirstSamplesMs = 150;
}  // namespace

namespace coral::micro {
namespace arduino {

class PDMClass {
   public:
    PDMClass();
    ~PDMClass();

    int begin();
    void end();

    int available();
    int read(std::vector<int32_t>& buffer, size_t size);

    void onReceive(void (*)(void));

    // Not Implemented
    void setGain(int gain);

    // Not Implemented
    void setBufferSize(int bufferSize);

   private:
    void Append(const int32_t* samples, size_t num_samples);

    AudioDriver driver_;
    AudioDriverConfig config_;
    AudioService audio_service_;
    LatestSamples latest_samples_;
    std::optional<int> current_audio_cb_id_;

    void (*onReceive_)(void);
};

}  // namespace arduino
}  // namespace coral::micro

extern coral::micro::arduino::PDMClass Mic;

#endif  // PDM_H_