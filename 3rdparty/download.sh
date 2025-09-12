#!/bin/bash
set -euo pipefail
if [ ! -d "repo/mbedtls" ]; then
    git clone https://github.com/Mbed-TLS/mbedtls.git repo/mbedtls --recursive --depth 1 -b mbedtls-3.6.4
else
    echo "mbedtls already exists, skipping clone."
fi

if [ ! -d "repo/libdatachannel" ]; then
    git clone https://github.com/paullouisageneau/libdatachannel.git repo/libdatachannel --recursive --depth 1 -b v0.23.1
else
    echo "libdatachannel already exists, skipping clone."
fi

if [ ! -d "repo/opus" ]; then
    git clone https://github.com/xiph/opus repo/opus --recursive --depth 1 -b v1.5.2
else
    echo "opus already exists, skipping clone."
fi

if [ ! -d "repo/FFmpeg" ]; then
    git clone https://github.com/FFmpeg/FFmpeg.git repo/ffmpeg --recursive --depth 1 -b n7.1.1
else
    echo "FFmpeg already exists, skipping clone."
fi