#pragma once

using AudioProcessCallback = std::function<oboe::DataCallbackResult(
        const void* inputData,
        int numInputFrames,
        void* outputData,
        int numOutputFrames
)>;

class AudioDataCallback : public oboe::FullDuplexStream {
public:
    explicit AudioDataCallback(AudioProcessCallback processCallback)
        : mProcessCallback(std::move(processCallback)) {}

    oboe::DataCallbackResult onBothStreamsReady(
            const void* inputData,
            int numInputFrames,
            void* outputData,
            int numOutputFrames) override {

        if (mProcessCallback) {
            return mProcessCallback(inputData, numInputFrames, outputData, numOutputFrames);
        }

        // If no callback, just pass silence
        memset(outputData, 0, numOutputFrames * sizeof(float));
        return oboe::DataCallbackResult::Continue;
    }

    void setProcessCallback(AudioProcessCallback callback) {
        mProcessCallback = std::move(callback);
    }

private:
    AudioProcessCallback mProcessCallback;
};
