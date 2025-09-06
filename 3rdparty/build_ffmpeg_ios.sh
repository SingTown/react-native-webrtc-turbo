#!/bin/bash
set -euo pipefail
SDKS=("iphoneos" "iphonesimulator" "iphonesimulator")
ARCHS=("arm64" "arm64" "x86_64")
FLAGS=("-miphoneos-version-min=15.1" "-mios-simulator-version-min=15.1" "-mios-simulator-version-min=15.1")

for i in "${!SDKS[@]}"; do
    SDK="${SDKS[$i]}"
    ARCH="${ARCHS[$i]}"
    FLAG="${FLAGS[$i]}"

    (
        mkdir -p build/ffmpeg/$SDK/$ARCH
        cd build/ffmpeg/$SDK/$ARCH
        ../../../../repo/ffmpeg/configure \
            --sysroot="$(xcrun --sdk $SDK --show-sdk-path)" \
            --enable-cross-compile --arch=$ARCH \
            --prefix=install \
            --cc="clang -arch $ARCH" \
            --cxx="clang++ -arch $ARCH" \
            --extra-cflags="$FLAG" \
            --extra-ldflags="$FLAG" \
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
            --enable-videotoolbox \
            --disable-mediacodec \
            --enable-decoder=h264 \
            --enable-decoder=hevc \
            --enable-parser=h264 \
            --enable-parser=hevc \
            --enable-encoder=h264_videotoolbox \
            --enable-encoder=hevc_videotoolbox

        make -j install
        libtool -static -o libffmpeg.a \
            install/lib/libavcodec.a \
            install/lib/libswscale.a \
            install/lib/libavutil.a
    )
done

libtool -static \
    build/ffmpeg/iphoneos/arm64/libffmpeg.a \
    -o build/ffmpeg/iphoneos/libffmpeg.a

libtool -static \
    build/ffmpeg/iphonesimulator/arm64/libffmpeg.a \
    build/ffmpeg/iphonesimulator/x86_64/libffmpeg.a \
    -o build/ffmpeg/iphonesimulator/libffmpeg.a

rm -rf output/ios/ffmpeg.xcframework
xcodebuild -create-xcframework \
  -library build/ffmpeg/iphoneos/libffmpeg.a \
  -headers build/ffmpeg/iphoneos/arm64/install/include \
  -library build/ffmpeg/iphonesimulator/libffmpeg.a \
  -headers build/ffmpeg/iphonesimulator/arm64/install/include \
  -output output/ios/ffmpeg.xcframework