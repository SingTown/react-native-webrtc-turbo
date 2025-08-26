#!/bin/bash
ARCHS=("armv7" "aarch64" "x86" "x86_64")
CPUS=("armv7-a" "armv8-a" "i686" "x86-64")
ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")
TOOLCHAIN_ARCHS=("armv7a-linux-androideabi" "aarch64-linux-android" "i686-linux-android" "x86_64-linux-android")

NDK_VERSION="27.1.12297006"
yes | sdkmanager --install "ndk;${NDK_VERSION}"

for i in "${!ARCHS[@]}"; do
    ABI="${ABIS[$i]}"
    ARCH="${ARCHS[$i]}"
    CPU="${CPUS[$i]}"
    TOOLCHAIN_ARCH="${TOOLCHAIN_ARCHS[$i]}"

    (
        mkdir -p build/ffmpeg/android/$ABI
        cd build/ffmpeg/android/$ABI
        ../../../../repo/ffmpeg/configure \
            --host-os=darwin-x86_64 --target-os=android \
            --enable-cross-compile --arch=$ARCH --cpu=$CPU \
            --sysroot=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot \
            --sysinclude=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/ \
            --cc=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-clang \
            --cxx=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-clang++ \
            --strip=$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-strip \
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
            --enable-decoder=h264 \
            --enable-decoder=hevc \
            --enable-parser=h264 \
            --enable-parser=hevc

        make -j install
    )
done
