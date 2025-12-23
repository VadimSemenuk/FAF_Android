#include <jni.h>
#include <memory>
#include <mutex>
#include <android/log.h>
#include "soundtouch/include/SoundTouch.h"
#include "AudioEngine.h"

using namespace soundtouch;

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "test", __VA_ARGS__)

static std::unique_ptr<AudioEngine> engine;
static std::mutex engineMutex;

static AudioEngine* getEngine() {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!engine) {
        LOGD("reinstantiate");
        engine = std::make_unique<AudioEngine>();
    }
    return engine.get();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_start(JNIEnv*, jobject) {
    AudioEngine* e = getEngine();
    if (!e) {
        return JNI_FALSE;
    }

    bool result = e->start();
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_stop(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (engine) {
        engine->stop();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_setInputDeviceId(
        JNIEnv*, jobject, jint value) {
    AudioEngine* e = getEngine();
    if (e) e->setInputDeviceId(value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_resetInputDeviceId(JNIEnv*, jobject) {
    AudioEngine* e = getEngine();
    if (e) e->resetInputDeviceId();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_setOutputDeviceId(
        JNIEnv*, jobject, jint value) {
    AudioEngine* e = getEngine();
    if (e) e->setOutputDeviceId(value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_resetOutputDeviceId(JNIEnv*, jobject) {
    AudioEngine* e = getEngine();
    if (e) e->resetOutputDeviceId();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_setSampleRate(
        JNIEnv*, jobject, jint value) {
    AudioEngine* e = getEngine();
    if (e) e->setSampleRate(value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_setPitch(
        JNIEnv*, jobject, jfloat value) {
    AudioEngine* e = getEngine();
    if (e) e->setPitch(value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_setGain(
        JNIEnv*, jobject, jint value) {
    AudioEngine* e = getEngine();
    if (e) e->setGain(value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_setGainType(
        JNIEnv*, jobject, jint value) {
    AudioEngine* e = getEngine();
    if (e) e->setGainType(value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_startRecording(
        JNIEnv*, jobject, jint fd) {
    AudioEngine* e = getEngine();
    if (e) e->startRecording(fd);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_pragmatsoft_faf_services_audio_NativeWrapper_stopRecording(JNIEnv*, jobject) {
    AudioEngine* e = getEngine();
    if (e) e->stopRecording();
}