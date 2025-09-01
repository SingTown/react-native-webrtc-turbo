#!/bin/bash
set -euo pipefail
ARCHS=("armv7" "aarch64" "x86" "x86_64")
CPUS=("armv7-a" "armv8-a" "i686" "x86-64")
ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")
TOOLCHAIN_ARCHS=("armv7a-linux-androideabi" "aarch64-linux-android" "i686-linux-android" "x86_64-linux-android")

NDK_VERSION="27.1.12297006"

for i in "${!ARCHS[@]}"; do
    ABI="${ABIS[$i]}"
    ARCH="${ARCHS[$i]}"
    CPU="${CPUS[$i]}"
    TOOLCHAIN_ARCH="${TOOLCHAIN_ARCHS[$i]}"
    FFMPEG_DIR="build/ffmpeg/android/$ABI"
    OUTPUT_DIR="output/android/ffmpeg/$ABI"

    (
        mkdir -p $FFMPEG_DIR
        cd $FFMPEG_DIR
        ../../../../repo/ffmpeg/configure \
            --host-os=darwin-x86_64 --target-os=android \
            --enable-cross-compile --arch=$ARCH --cpu=$CPU \
            --sysroot=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot \
            --sysinclude=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/ \
            --cc=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-clang \
            --cxx=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-clang++ \
            --strip=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-strip \
            --prefix=install \
            --disable-everything \
            --enable-shared --disable-static \
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
            --enable-mediacodec \
            --enable-jni \
            --enable-decoder=h264 \
            --enable-decoder=hevc \
            --enable-parser=h264 \
            --enable-parser=hevc \
            --enable-encoder=h264_mediacodec

        make -j install
    )

    (
        rm -rf $OUTPUT_DIR
        mkdir -p $OUTPUT_DIR/lib
        cp -r $FFMPEG_DIR/install/lib/*.so $OUTPUT_DIR/lib
        cp -r $FFMPEG_DIR/install/include $OUTPUT_DIR/include
    )
done
