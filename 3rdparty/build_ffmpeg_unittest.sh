#!/bin/bash
set -euo pipefail
TOP_DIR="$(cd "$(dirname "$0")" && pwd)"
OPUS_DIR="${TOP_DIR}/build/opus/unittest"
FFMPEG_DIR="${TOP_DIR}/build/ffmpeg/unittest"
OUTPUT_DIR="${TOP_DIR}/output/unittest/ffmpeg"

cmake -B $OPUS_DIR \
    -DCMAKE_INSTALL_PREFIX=$OPUS_DIR/install \
    -DCMAKE_BUILD_TYPE=Release \
    repo/opus
cmake --build $OPUS_DIR --config Release --target install

(
    mkdir -p $FFMPEG_DIR
    cd $FFMPEG_DIR
    export PKG_CONFIG_PATH="$OPUS_DIR/install/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
    ../../../repo/ffmpeg/configure \
        --prefix=install \
        --pkg-config-flags="--static" \
        --extra-cflags="-I$OPUS_DIR/install/include" \
        --extra-ldflags="-L$OPUS_DIR/install/lib" \
        --disable-everything \
        --disable-shared --enable-static \
        --disable-asm \
        --disable-iconv \
        --disable-avformat \
        --disable-avdevice \
        --disable-avfilter \
        --enable-swresample \
        --enable-avcodec \
        --enable-swscale \
        --enable-avutil \
        --disable-audiotoolbox \
        --disable-videotoolbox \
        --disable-mediacodec \
        --enable-libopus \
        --disable-jni \
        --enable-decoder=h264 \
        --enable-decoder=hevc \
        --enable-parser=h264 \
        --enable-parser=hevc \
        --enable-decoder=libopus \
        --enable-encoder=libopus \
        --enable-parser=opus

    make -j install
)

(
    rm -rf $OUTPUT_DIR
    mkdir -p $OUTPUT_DIR/lib
    cp -r $OPUS_DIR/install/lib/*.a $OUTPUT_DIR/lib
    cp -r $FFMPEG_DIR/install/lib/*.a $OUTPUT_DIR/lib
    cp -r $FFMPEG_DIR/install/include $OUTPUT_DIR/include
)
