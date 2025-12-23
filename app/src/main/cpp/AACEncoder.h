#pragma once
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>
#include <thread>
#include <atomic>
#include "PcmRingBuffer.h"


class AacEncoder {
public:
    AacEncoder(PcmRingBuffer& buffer,
               int sampleRate,
               int channels,
               int fd);


    void start();
    void stop();

    ~AacEncoder() {
        stop();
    }

private:
    void encodeLoop();

    PcmRingBuffer& mRing;
    int mSampleRate;
    int mChannels;
    int mFd;


    std::thread mThread;
    std::atomic<bool> mRunning{false};
};