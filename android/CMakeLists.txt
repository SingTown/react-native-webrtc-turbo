cmake_minimum_required(VERSION 3.4.1)
project(DatachannelTurboModule)

set (CMAKE_VERBOSE_MAKEFILE ON)

set(MBEDTLS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/build/mbedtls/android-${ANDROID_ABI}/install)
set(LIBDATACHANNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/build/libdatachannel/android-${ANDROID_ABI}/install)

add_library(
    react-native-webrtc-turbo
    STATIC
    ../cpp/DatachannelImpl.cpp
)

# Specifies a path to native header files.
target_include_directories(
    react-native-webrtc-turbo
    PUBLIC
    ${MBEDTLS_PATH}/include
    ${LIBDATACHANNEL_PATH}/include
    ../cpp
)

target_link_libraries(
    react-native-webrtc-turbo
    jsi
    reactnative
    react_codegen_RNDatachannelTurboModuleSpec
    ${LIBDATACHANNEL_PATH}/lib/libdatachannel.a
    ${LIBDATACHANNEL_PATH}/lib/libjuice.a
    ${LIBDATACHANNEL_PATH}/lib/libsrtp2.a
    ${LIBDATACHANNEL_PATH}/lib/libusrsctp.a
    ${MBEDTLS_PATH}/lib/libmbedcrypto.a
    ${MBEDTLS_PATH}/lib/libmbedx509.a
    ${MBEDTLS_PATH}/lib/libmbedtls.a
)