cmake_minimum_required(VERSION 3.14)
project(testcpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (TOP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(LIBDATACHANNEL_PATH ${TOP_PATH}/3rdparty/output/unittest/libdatachannel)
set(FFMPEG_PATH ${TOP_PATH}/3rdparty/output/unittest/ffmpeg)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)
enable_testing()

file(GLOB CPP_SOURCES
    ${TOP_PATH}/cpp/__tests__/*.cpp
    ${TOP_PATH}/cpp/*.cpp
)

add_executable(
    testcpp
    ${CPP_SOURCES}
)

target_include_directories(
    testcpp
    PUBLIC
    ${TOP_PATH}/cpp/
    ${TOP_PATH}/cpp/__tests__/
    ${LIBDATACHANNEL_PATH}/include
    ${FFMPEG_PATH}/include
)

target_link_libraries(
    testcpp
    PRIVATE
    ${LIBDATACHANNEL_PATH}/lib/libdatachannel.a
    ${LIBDATACHANNEL_PATH}/lib/libjuice.a
    ${LIBDATACHANNEL_PATH}/lib/libsrtp2.a
    ${LIBDATACHANNEL_PATH}/lib/libusrsctp.a
    ${LIBDATACHANNEL_PATH}/lib/libmbedtls.a
    ${LIBDATACHANNEL_PATH}/lib/libmbedx509.a
    ${LIBDATACHANNEL_PATH}/lib/libmbedcrypto.a
    ${FFMPEG_PATH}/lib/libavcodec.a
    ${FFMPEG_PATH}/lib/libavutil.a
    ${FFMPEG_PATH}/lib/libswscale.a
    ${FFMPEG_PATH}/lib/libswresample.a
    opus
    x264
    x265
    gtest
    gtest_main
)