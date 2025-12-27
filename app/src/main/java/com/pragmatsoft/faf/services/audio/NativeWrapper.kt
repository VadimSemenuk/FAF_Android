package com.pragmatsoft.faf.services.audio

object NativeWrapper {

    init {
        System.loadLibrary("native-lib")
    }

    external fun start(): Boolean
    external fun stop()

    external fun setInputDeviceId(value: Int)
    external fun resetInputDeviceId()
    external fun setOutputDeviceId(value: Int)
    external fun resetOutputDeviceId()
    external fun setSampleRate(value: Int)
    external fun setPitch(value: Float)
    external fun setGain(value: Int)
    external fun setGainType(value: Int)

    external fun startRecording(fd: Int)
    external fun stopRecording()
}