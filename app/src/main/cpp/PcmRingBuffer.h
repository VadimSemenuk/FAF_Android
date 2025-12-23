#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <atomic>
#include <algorithm>

class PcmRingBuffer {
public:
    PcmRingBuffer() : PcmRingBuffer(4096) {}

    explicit PcmRingBuffer(size_t capacity)
            : buffer(capacity), capacity(capacity) {}

    bool push(const float* data, size_t count) {
        size_t free = capacity - size();
        if (count > free) return false;

        for (size_t i = 0; i < count; ++i) {
            buffer[writeIndex.load(std::memory_order_relaxed)] = data[i];
            writeIndex.store((writeIndex.load(std::memory_order_relaxed) + 1) % capacity,
                             std::memory_order_relaxed);
        }
        writeCounter.fetch_add(count, std::memory_order_release);
        return true;
    }

    size_t pop(float* out, size_t count) {
        size_t available = size();
        size_t toRead = std::min(count, available);

        for (size_t i = 0; i < toRead; ++i) {
            out[i] = buffer[readIndex.load(std::memory_order_relaxed)];
            readIndex.store((readIndex.load(std::memory_order_relaxed) + 1) % capacity,
                            std::memory_order_relaxed);
        }
        readCounter.fetch_add(toRead, std::memory_order_release);
        return toRead;
    }

    size_t size() const {
        return writeCounter.load(std::memory_order_acquire) -
               readCounter.load(std::memory_order_acquire);
    }

    void clear() {
        readCounter.store(writeCounter.load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
        readIndex.store(writeIndex.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
    }

private:
    std::vector<float> buffer;
    size_t capacity;
    std::atomic<size_t> writeCounter{0};
    std::atomic<size_t> readCounter{0};
    std::atomic<size_t> writeIndex{0};
    std::atomic<size_t> readIndex{0};
};