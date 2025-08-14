cmake_minimum_required(VERSION 3.4.1)
project(datachannelModule)

set (CMAKE_VERBOSE_MAKEFILE ON)

add_library(
    datachannel-module
    STATIC
    ../cpp/NativeDatachannelModule.cpp
)

# Specifies a path to native header files.
target_include_directories(
    datachannel-module
    PUBLIC
    ../cpp
)

target_link_libraries(
    datachannel-module
    jsi
    reactnative
    react_codegen_WebrtcSpec
)