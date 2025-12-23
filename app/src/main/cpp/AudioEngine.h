#pragma once

#include <oboe/Oboe.h>
#include <fstream>
#include <vector>
#include <mutex>
#include <atomic>
#include "soundtouch/include/SoundTouch.h"
#include "PcmRingBuffer.h"
#include "AACEncoder.h"
#include "AudioDataCallback.h"
#include "AudioStreamErrorHandler.h"
#include "GainProcessor.h"

using namespace soundtouch;

class AudioEngine {

public:
    AudioEngine();
    ~AudioEngine();

    bool start();
    void stop();

    void setInputDeviceId(int value);
    void resetInputDeviceId();
    void setOutputDeviceId(int value);
    void resetOutputDeviceId();
    void setSampleRate(int value);
    void setPitch(float value);
    void setGain(int value);
    void setGainType(int value);

    void startRecording(int fd);
    void stopRecording();

    oboe::DataCallbackResult processAudio(
            const void *inputData,
            int numInputFrames,
            void *outputData,
            int numOutputFrames);

    void handleStreamError(oboe::AudioStream* stream, oboe::Result error);

private:
    SoundTouch soundTouch;
    PcmRingBuffer ringBuffer;
    std::unique_ptr<GainProcessor> gainProcessor;

    std::unique_ptr<AacEncoder> encoder;
    std::mutex encoderMutex;

    std::unique_ptr<AudioDataCallback> dataCallback;
    std::unique_ptr<AudioStreamErrorHandler> errorHandler;

    std::shared_ptr<oboe::AudioStream> inputStream;
    std::shared_ptr<oboe::AudioStream> outputStream;

    int sampleRate = oboe::kUnspecified;
    int inputDeviceId = oboe::kUnspecified;
    int outputDeviceId = oboe::kUnspecified;
    std::atomic<float> pitch{1.0f};
    int gainProcessorType = 0;

    void initCallbacks();
    void initGainProcessor();
    void setupSoundTouch();
    void setupGainProcessor(int sr);
    void cleanupStreams();
};