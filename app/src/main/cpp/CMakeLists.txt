cmake_minimum_required(VERSION 3.22.1)

project("native-lib")

add_subdirectory(soundtouch)

find_package (oboe REQUIRED CONFIG)

add_library(
        native-lib SHARED
        native-lib.cpp
        AudioEngine.cpp
        AACEncoder.cpp
        PcmRingBuffer.h
)

find_library(log-lib log)

target_link_libraries(
        native-lib
        SoundTouch
        oboe::oboe
        android
        mediandk
        ${log-lib}
)

set(CMAKE_BUILD_TYPE RelWithDebInfo)