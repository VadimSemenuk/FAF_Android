#pragma once

using AudioErrorCallback = std::function<void(
        oboe::AudioStream* stream,
        oboe::Result error
)>;

class AudioStreamErrorHandler : public oboe::AudioStreamErrorCallback {
public:
    explicit AudioStreamErrorHandler(AudioErrorCallback errorCallback)
            : mErrorCallback(std::move(errorCallback)) {}

    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override {
        if (mErrorCallback) {
            mErrorCallback(stream, error);
        }
    }

    void setErrorCallback(AudioErrorCallback callback) {
        mErrorCallback = std::move(callback);
    }

private:
    AudioErrorCallback mErrorCallback;
};
