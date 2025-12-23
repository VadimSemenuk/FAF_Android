#include "AudioEngine.h"
#include "AACEncoder.h"
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "test", __VA_ARGS__)

AudioEngine::AudioEngine() {
    initCallbacks();
    initGainProcessor();
}

AudioEngine::~AudioEngine() {
    stop();
}

void AudioEngine::initCallbacks() {
    dataCallback = std::make_unique<AudioDataCallback>(
            [this](const void* inputData, int numInputFrames,
                   void* outputData, int numOutputFrames) {
                return this->processAudio(inputData, numInputFrames,
                                          outputData, numOutputFrames);
            }
    );

    errorHandler = std::make_unique<AudioStreamErrorHandler>(
            [this](oboe::AudioStream* stream, oboe::Result error) {
                this->handleStreamError(stream, error);
            }
    );
}

void AudioEngine::initGainProcessor() {
    if (gainProcessorType == 1) {
        gainProcessor = std::make_unique<NoiseReductionGainProcessor>();
    } else {
        gainProcessor = std::make_unique<PlainGainProcessor>();
    }
}

bool AudioEngine::start() {
    oboe::AudioStreamBuilder outputBuilder;
    outputBuilder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setUsage(oboe::Usage::AssistanceAccessibility)
            ->setContentType(oboe::ContentType::Speech)
            ->setSampleRate(sampleRate)
            ->setChannelCount(1)
            ->setChannelMask(oboe::ChannelMask::Mono)
            ->setDeviceId(outputDeviceId)
            ->setDataCallback(dataCallback.get())
            ->setErrorCallback(errorHandler.get());

    oboe::Result outputStreamOpenResult = outputBuilder.openStream(outputStream);

    if (outputStreamOpenResult != oboe::Result::OK) {
        return false;
    }

    oboe::AudioStreamBuilder inputBuilder;
    inputBuilder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
//            ->setInputPreset(oboe::InputPreset::Generic)
            ->setSampleRate(outputStream->getSampleRate())
            ->setChannelCount(1)
            ->setChannelMask(oboe::ChannelMask::Mono)
            ->setDeviceId(inputDeviceId)
            ->setBufferCapacityInFrames(outputStream->getBufferCapacityInFrames() * 2);

    oboe::Result inputStreamOpenResult = inputBuilder.openStream(inputStream);

    if (inputStreamOpenResult != oboe::Result::OK) {
        cleanupStreams();
        return false;
    }

    dataCallback->setSharedInputStream(inputStream);
    dataCallback->setSharedOutputStream(outputStream);

    oboe::Result startResult = dataCallback->start();
    if (startResult != oboe::Result::OK) {
        cleanupStreams();
        return false;
    }
    setupSoundTouch();
    setupGainProcessor(outputStream->getSampleRate());

    return true;
}

void AudioEngine::setupSoundTouch() {
    soundTouch.setSampleRate(outputStream->getSampleRate());
    soundTouch.setChannels(1);
    soundTouch.setPitch(pitch.load(std::memory_order_relaxed));
    soundTouch.setTempo(1.0f);

    soundTouch.setSetting(SETTING_USE_QUICKSEEK, 1);
    soundTouch.setSetting(SETTING_USE_AA_FILTER, 0);
    soundTouch.setSetting(SETTING_SEQUENCE_MS, 20);
    soundTouch.setSetting(SETTING_SEEKWINDOW_MS, 10);
    soundTouch.setSetting(SETTING_OVERLAP_MS, 5);

    soundTouch.clear();
}

void AudioEngine::setupGainProcessor(int sr) {
    if (auto* pg = dynamic_cast<NoiseReductionGainProcessor*>(gainProcessor.get())) {
        pg->setSampleRate(static_cast<float>(sr));
    }
}

void AudioEngine::cleanupStreams() {
    if (outputStream) {
        outputStream->stop();
        outputStream->close();
        outputStream = nullptr;
    }
    if (inputStream) {
        inputStream->stop();
        inputStream->close();
        inputStream = nullptr;
    }
}

void AudioEngine::stop() {
    stopRecording();

    dataCallback->stop();

    cleanupStreams();

    ringBuffer.clear();
    soundTouch.clear();
}

oboe::DataCallbackResult AudioEngine::processAudio(
        const void *inputData,
        int numInputFrames,
        void *outputData,
        int numOutputFrames) {

    auto *input = static_cast<const float *>(inputData);
    auto *output = static_cast<float *>(outputData);

    std::vector<float> gainedInput(numInputFrames);
    for (int i = 0; i < numInputFrames; ++i) {
        gainedInput[i] = gainProcessor->process(input[i]);
    }

    soundTouch.putSamples(gainedInput.data(), numInputFrames);
    uint numReceived = soundTouch.receiveSamples(output, numOutputFrames);

    if (numReceived < numOutputFrames) {
        std::fill(output + numReceived, output + numOutputFrames, 0.0f);
    }

    {
        std::lock_guard<std::mutex> lock(encoderMutex);
        if (encoder) {
            ringBuffer.push(gainedInput.data(), numInputFrames);
        }
    }

//    int framesToProcess = std::min(numInputFrames, numOutputFrames);
//    int bytesPerSample = getInputStream()->getBytesPerSample();
//    memcpy(outputData, gainedInput, framesToProcess * bytesPerSample);

    return oboe::DataCallbackResult::Continue;
}

void AudioEngine::handleStreamError(oboe::AudioStream* stream, oboe::Result error) {
    stop();
}

void AudioEngine::setInputDeviceId(int value) {
    inputDeviceId = value;
}

void AudioEngine::resetInputDeviceId() {
    inputDeviceId = oboe::kUnspecified;
}

void AudioEngine::setOutputDeviceId(int value) {
    outputDeviceId = value;
}

void AudioEngine::resetOutputDeviceId() {
    outputDeviceId = oboe::kUnspecified;
}

void AudioEngine::setSampleRate(int value) {
    sampleRate = value;
}

void AudioEngine::setPitch(float value) {
    pitch.store(value, std::memory_order_relaxed);

    soundTouch.clear();
    soundTouch.flush();
    soundTouch.setPitch(value);
}

void AudioEngine::setGain(int value) {
    gainProcessor->setGain(value);
}

void AudioEngine::setGainType(int value) {
    gainProcessorType = value;
    initGainProcessor();
}

void AudioEngine::startRecording(int fd) {
    std::lock_guard<std::mutex> lock(encoderMutex);

    if (encoder) {
        encoder->stop();
    }

    ringBuffer.clear();

    encoder = std::make_unique<AacEncoder>(
            ringBuffer,
            outputStream->getSampleRate(),
            1,
            fd
    );
    encoder->start();
}

void AudioEngine::stopRecording() {
    std::lock_guard<std::mutex> lock(encoderMutex);

    if (encoder) {
        encoder->stop();
        encoder = nullptr;
    }
}