cmake_minimum_required(VERSION 3.4.1)
project(datachannelModule)

set (CMAKE_VERBOSE_MAKEFILE ON)

set(LIBDATACHANNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/output/android/libdatachannel/${ANDROID_ABI})
set(FFMPEG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/output/android/ffmpeg/${ANDROID_ABI})

file(GLOB CPP_SOURCES "../cpp/*.cpp" "src/main/cpp/*.cpp")

add_library(
    datachannel-module
    SHARED
    ${CPP_SOURCES}
)

# Specifies a path to native header files.
target_include_directories(
    datachannel-module
    PUBLIC
    ../cpp
    src/main/cpp
    ${LIBDATACHANNEL_PATH}/include
    ${FFMPEG_PATH}/include
)

target_link_libraries(
    datachannel-module
    jsi
    jnigraphics
    log
    reactnative
    react_codegen_WebrtcSpec
    ${LIBDATACHANNEL_PATH}/lib/libdatachannel.so
    ${FFMPEG_PATH}/lib/libavcodec.so
    ${FFMPEG_PATH}/lib/libavutil.so
    ${FFMPEG_PATH}/lib/libswscale.so
)