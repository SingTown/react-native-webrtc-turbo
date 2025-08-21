cmake_minimum_required(VERSION 3.4.1)
project(datachannelModule)

set (CMAKE_VERBOSE_MAKEFILE ON)

set(MBEDTLS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/build/mbedtls/android-${ANDROID_ABI}/install)
set(LIBDATACHANNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/build/libdatachannel/android-${ANDROID_ABI}/install)
set(FFMPEG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/build/ffmpeg/android-${ANDROID_ABI}/install)

add_library(
    datachannel-module
    STATIC
    ../cpp/NativeDatachannelModule.cpp
    ../cpp/NativeDatachannelModuleAndroid.cpp
)

# Specifies a path to native header files.
target_include_directories(
    datachannel-module
    PUBLIC
    ../cpp
    ${MBEDTLS_PATH}/include
    ${LIBDATACHANNEL_PATH}/include
    ${FFMPEG_PATH}/include
)

target_link_libraries(
    datachannel-module
    jsi
    reactnative
    react_codegen_WebrtcSpec
    ${LIBDATACHANNEL_PATH}/lib/libdatachannel.a
    ${LIBDATACHANNEL_PATH}/lib/libjuice.a
    ${LIBDATACHANNEL_PATH}/lib/libsrtp2.a
    ${LIBDATACHANNEL_PATH}/lib/libusrsctp.a
    ${MBEDTLS_PATH}/lib/libmbedcrypto.a
    ${MBEDTLS_PATH}/lib/libmbedx509.a
    ${MBEDTLS_PATH}/lib/libmbedtls.a
    ${FFMPEG_PATH}/lib/libavcodec.a
    ${FFMPEG_PATH}/lib/libavutil.a
)