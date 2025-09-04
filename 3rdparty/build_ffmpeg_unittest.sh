#!/bin/bash
set -euo pipefail
FFMPEG_DIR="build/ffmpeg/unittest"
OUTPUT_DIR="output/unittest/ffmpeg"

(
    mkdir -p $FFMPEG_DIR
    cd $FFMPEG_DIR
    ../../../repo/ffmpeg/configure \
        --prefix=install \
        --disable-everything \
        --disable-shared --enable-static \
        --disable-asm \
        --disable-iconv \
        --disable-avformat \
        --disable-avdevice \
        --disable-avfilter \
        --disable-swresample \
        --enable-avcodec \
        --enable-swscale \
        --enable-avutil \
        --disable-audiotoolbox \
        --disable-videotoolbox \
        --disable-mediacodec \
        --disable-jni \
        --enable-decoder=h264 \
        --enable-decoder=hevc \
        --enable-parser=h264 \
        --enable-parser=hevc 

    make -j install
)

(
    rm -rf $OUTPUT_DIR
    mkdir -p $OUTPUT_DIR/lib
    cp -r $FFMPEG_DIR/install/lib/*.a $OUTPUT_DIR/lib
    cp -r $FFMPEG_DIR/install/include $OUTPUT_DIR/include
)
