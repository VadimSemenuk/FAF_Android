#include "AACEncoder.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <android/log.h>

#ifndef AMEDIAFORMAT_AAC_PROFILE_LC
#define AMEDIAFORMAT_AAC_PROFILE_LC 2
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "AACEncoder", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "AACEncoder", __VA_ARGS__)

static constexpr int AAC_FRAME_SAMPLES = 1024;
static constexpr int TIMEOUT_US = 10000;  // 10ms timeout

AacEncoder::AacEncoder(PcmRingBuffer& buffer,
                       int sampleRate,
                       int channels,
                       int fd)
        : mRing(buffer),
          mSampleRate(sampleRate),
          mChannels(channels),
          mFd(fd) {}


void AacEncoder::start() {
    if (mRunning.load()) {
        LOGE("Encoder already running");
        return;
    }

    mRunning.store(true);
    mThread = std::thread(&AacEncoder::encodeLoop, this);
}


void AacEncoder::stop() {
    if (!mRunning.load()) {
        return;
    }

    mRunning.store(false);
    if (mThread.joinable()) {
        mThread.join();
    }
}

void AacEncoder::encodeLoop() {
    LOGI("encodeLoop started");

    AMediaCodec* codec = AMediaCodec_createEncoderByType("audio/mp4a-latm");
    if (!codec) {
        LOGE("Failed to create AAC encoder");
        return;
    }

    AMediaFormat* format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, "audio/mp4a-latm");
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, mSampleRate);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, mChannels);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, 128000);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_AAC_PROFILE,
                          AMEDIAFORMAT_AAC_PROFILE_LC);
//    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_PCM_ENCODING, 2);

    media_status_t status = AMediaCodec_configure(codec, format, nullptr, nullptr,
                                                  AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    AMediaFormat_delete(format);

    if (status != AMEDIA_OK) {
        LOGE("Failed to configure codec: %d", status);
        AMediaCodec_delete(codec);
        return;
    }

    status = AMediaCodec_start(codec);
    if (status != AMEDIA_OK) {
        LOGE("Failed to start codec: %d", status);
        AMediaCodec_delete(codec);
        return;
    }

    AMediaMuxer* muxer = AMediaMuxer_new(mFd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    if (!muxer) {
        LOGE("Failed to create muxer");
        AMediaCodec_stop(codec);
        AMediaCodec_delete(codec);
        return;
    }

    bool muxerStarted = false;
    int trackIndex = -1;

    std::vector<float> floatBuf(AAC_FRAME_SAMPLES * mChannels);
    std::vector<int16_t> pcm16(floatBuf.size());

    int64_t ptsUs = 0;
    int64_t frameUs = AAC_FRAME_SAMPLES * 1000000LL / mSampleRate;
    bool eosSignaled = false;

    while (mRunning.load() || !eosSignaled) {
        // 1. Feed input if we have enough PCM
        if (mRunning.load() && mRing.size() >= AAC_FRAME_SAMPLES) {
            size_t read = mRing.pop(floatBuf.data(), AAC_FRAME_SAMPLES);

            if (read == AAC_FRAME_SAMPLES) {
                // Convert float to int16
                for (size_t i = 0; i < AAC_FRAME_SAMPLES; ++i) {
                    float s = std::max(-1.0f, std::min(1.0f, floatBuf[i]));
                    pcm16[i] = static_cast<int16_t>(s * 32767.0f);
                }

                ssize_t inIdx = AMediaCodec_dequeueInputBuffer(codec, TIMEOUT_US);
                if (inIdx >= 0) {
                    size_t inSize;
                    uint8_t* inBuf = AMediaCodec_getInputBuffer(codec, inIdx, &inSize);

                    size_t dataSize = pcm16.size() * sizeof(int16_t);
                    if (inBuf && dataSize <= inSize) {
                        memcpy(inBuf, pcm16.data(), dataSize);

                        AMediaCodec_queueInputBuffer(
                                codec,
                                inIdx,
                                0,
                                dataSize,
                                ptsUs,
                                0);

                        ptsUs += frameUs;
                    } else {
                        LOGE("Input buffer too small or null");
                    }
                }
            }
        } else if (!mRunning.load() && !eosSignaled) {
            // Signal end of stream
            ssize_t inIdx = AMediaCodec_dequeueInputBuffer(codec, TIMEOUT_US);
            if (inIdx >= 0) {
                AMediaCodec_queueInputBuffer(
                        codec,
                        inIdx,
                        0,
                        0,
                        ptsUs,
                        AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                eosSignaled = true;
                LOGI("EOS signaled to encoder");
            }
        }

        // 2. Drain output
        AMediaCodecBufferInfo info;
        ssize_t outIdx = AMediaCodec_dequeueOutputBuffer(codec, &info, TIMEOUT_US);

        if (outIdx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            AMediaFormat* outFormat = AMediaCodec_getOutputFormat(codec);
            trackIndex = AMediaMuxer_addTrack(muxer, outFormat);
            AMediaFormat_delete(outFormat);

            if (trackIndex < 0) {
                LOGE("Failed to add track to muxer");
                break;
            }

            media_status_t muxerStatus = AMediaMuxer_start(muxer);
            if (muxerStatus != AMEDIA_OK) {
                LOGE("Failed to start muxer: %d", muxerStatus);
                break;
            }

            muxerStarted = true;
            LOGI("Muxer started, track index: %d", trackIndex);
        } else if (outIdx >= 0) {
            if (muxerStarted && info.size > 0) {
                size_t outSize;
                uint8_t* outBuf = AMediaCodec_getOutputBuffer(codec, outIdx, &outSize);

                if (outBuf) {
                    media_status_t writeStatus = AMediaMuxer_writeSampleData(
                            muxer,
                            trackIndex,
                            outBuf,
                            &info);

                    if (writeStatus != AMEDIA_OK) {
                        LOGE("Failed to write sample data: %d", writeStatus);
                    }
                }
            }

            AMediaCodec_releaseOutputBuffer(codec, outIdx, false);

            // Check for end of stream
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                LOGI("Received EOS from encoder");
                break;
            }
        } else if (outIdx == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            // No output available yet
            if (!mRunning.load() && eosSignaled) {
                // Give it more time to flush
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }

    LOGI("Encoding loop finished, cleaning up");

    // Cleanup
    AMediaCodec_stop(codec);
    AMediaCodec_delete(codec);

    if (muxerStarted) {
        AMediaMuxer_stop(muxer);
    }
    AMediaMuxer_delete(muxer);

    LOGI("Encoder cleanup complete");
}