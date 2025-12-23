#pragma once
#include <cmath>
#include <algorithm>

class GainProcessor {
public:
    virtual ~GainProcessor() = default;

//    std::atomic<float> gain{1};
    float gain = 1.0f;

    void setGain(int value) {
//        gain.store(value, std::memory_order_relaxed);
        gain = static_cast<float>(value);
    }

//    virtual void setSampleRate(float sr) { }

    virtual float process(float x) { return 0; }
};

class PlainGainProcessor : public GainProcessor {
public:
    float process(float x) override {
//    auto currentGain = gain.load(std::memory_order_relaxed);
        auto gainedSample = x * gain;
        return std::max(-1.0f, std::min(1.0f, gainedSample));
    }
};

//class NoiseReductionGainProcessor : public GainProcessor {
//public:
//    float process(float x) override {
//    }
//};

class NoiseReductionGainProcessor : public GainProcessor {
public:

    NoiseReductionGainProcessor() {
        setSampleRate(sampleRate);
    }

    void setSampleRate(float sr) {
        sampleRate = sr;

        // === High-pass biquad (80 Hz) ===
        const float fc = 80.0f;
        const float q  = 0.707f;
        float w0 = 2.0f * M_PI * fc / sampleRate;
        float cosw = cosf(w0);
        float sinw = sinf(w0);
        float alpha = sinw / (2.0f * q);

        float b0 =  (1 + cosw) / 2;
        float b1 = -(1 + cosw);
        float b2 =  (1 + cosw) / 2;
        float a0 =  1 + alpha;
        float a1 = -2 * cosw;
        float a2 =  1 - alpha;

        hp_b0 = b0 / a0;
        hp_b1 = b1 / a0;
        hp_b2 = b2 / a0;
        hp_a1 = a1 / a0;
        hp_a2 = a2 / a0;

        // === Envelope follower ===
        envAttack  = calcCoeff(5.0f);
        envRelease = calcCoeff(100.0f);
    }

    inline float process(float x) override {
        // === High-pass ===
        float y = hp_b0 * x + hp_z1;
        hp_z1 = hp_b1 * x - hp_a1 * y + hp_z2;
        hp_z2 = hp_b2 * x - hp_a2 * y;

        x = y;

        // === Envelope ===
        float absx = fabsf(x) + 1e-9f;
        if (absx > env)
            env = envAttack * env + (1 - envAttack) * absx;
        else
            env = envRelease * env + (1 - envRelease) * absx;

        float levelDb = 20.0f * log10f(env);

        // === Expander (noise reduction) ===
        float _gain = 1.0f;
        if (levelDb < expThreshold) {
            float g = (expThreshold - levelDb) * (1.0f - 1.0f / expRatio);
            g = std::min(g, expMaxReduction);
            _gain *= dbToLin(-g);
        }

        // === Compressor ===
        if (levelDb > compThreshold) {
            float g = (levelDb - compThreshold) * (1.0f - 1.0f / compRatio);
            _gain *= dbToLin(-g);
        }

        // === Makeup gain ===
        x *= _gain * gain;

        // === Hard limiter ===
        return std::max(-limit, std::min(x, limit));
    }

private:
    // === Utilities ===
    inline float calcCoeff(float ms) {
        return expf(-1.0f / (0.001f * ms * sampleRate));
    }

    inline float dbToLin(float db) {
        return expf(db * 0.115129254f); // ln(10)/20
    }

    // === Parameters (speech tuned) ===
    float sampleRate = 48000.0f;

    // Expander
    const float expThreshold = -40.0f;
    const float expRatio = 2.0f;
    const float expMaxReduction = 15.0f;

    // Compressor
    const float compThreshold = -20.0f;
    const float compRatio = 4.0f;

    // Output
    const float limit = 1.0f;

    // === High-pass state ===
    float hp_b0{}, hp_b1{}, hp_b2{}, hp_a1{}, hp_a2{};
    float hp_z1{}, hp_z2{};

    // === Envelope ===
    float env = 0.0f;
    float envAttack{}, envRelease{};
};
