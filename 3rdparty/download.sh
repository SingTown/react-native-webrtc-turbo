#!/bin/bash
git clone https://github.com/Mbed-TLS/mbedtls.git repo/mbedtls --recursive --depth 1 -b mbedtls-3.6.4
git clone https://github.com/paullouisageneau/libdatachannel.git repo/libdatachannel --recursive --depth 1 -b v0.23.1
git clone https://github.com/FFmpeg/FFmpeg.git repo/ffmpeg --recursive --depth 1 -b n7.1.1