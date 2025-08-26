cmake_minimum_required(VERSION 3.4.1)
project(datachannelModule)

set (CMAKE_VERBOSE_MAKEFILE ON)

set(LIBDATACHANNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/output/android/libdatachannel/${ANDROID_ABI})
set(FFMPEG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/output/android/ffmpeg/${ANDROID_ABI})

add_library(
    datachannel-module
    SHARED
    ../cpp/Decoder.cpp
    ../cpp/NativeDatachannelModule.cpp
    ../cpp/NativeDatachannelModuleAndroid.cpp
)

# Specifies a path to native header files.
target_include_directories(
    datachannel-module
    PUBLIC
    ../cpp
    ${LIBDATACHANNEL_PATH}/include
    ${FFMPEG_PATH}/include
)

target_link_libraries(
    datachannel-module
    jsi
    reactnative
    react_codegen_WebrtcSpec
    ${LIBDATACHANNEL_PATH}/lib/libdatachannel.so
    ${FFMPEG_PATH}/lib/libavcodec.so
    ${FFMPEG_PATH}/lib/libavutil.so
    ${FFMPEG_PATH}/lib/libswscale.so
)