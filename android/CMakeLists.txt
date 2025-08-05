cmake_minimum_required(VERSION 3.4.1)
project(DatachannelTurboModule)

set (CMAKE_VERBOSE_MAKEFILE ON)

add_library(
    react-native-webrtc-turbo
    STATIC
    ../cpp/DatachannelImpl.cpp
)

# Specifies a path to native header files.
target_include_directories(
    react-native-webrtc-turbo
    PUBLIC
    ../cpp
)

target_link_libraries(
    react-native-webrtc-turbo
    jsi
    reactnative
    react_codegen_RNDatachannelTurboModuleSpec
)