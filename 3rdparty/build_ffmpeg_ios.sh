SDKS=("iphoneos" "iphonesimulator")
FLAGS=("-miphoneos-version-min=15.1" "-mios-simulator-version-min=15.1")

for i in "${!SDKS[@]}"; do
    SDK="${SDKS[$i]}"
    FLAG="${FLAGS[$i]}"

    (
        mkdir -p build/ffmpeg/ios-$SDK-arm64
        cd build/ffmpeg/ios-$SDK-arm64
        ../../../ffmpeg/configure \
            --sysroot="$(xcrun --sdk $SDK --show-sdk-path)" \
            --enable-cross-compile --arch=arm64 \
            --prefix=install \
            --cc="xcrun --sdk $SDK clang -arch arm64" \
            --cxx="xcrun --sdk $SDK clang++ -arch arm64" \
            --extra-cflags="$FLAG" \
            --extra-ldflags="$FLAG" \
            --disable-everything \
            --disable-shared --enable-static \
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
        libtool -static -o install/lib/libffmpeg.a \
            install/lib/libavcodec.a \
            install/lib/libswscale.a \
            install/lib/libavutil.a
    )
done

rm -rf ffmpeg.xcframework
xcodebuild -create-xcframework \
  -library build/ffmpeg/ios-iphoneos-arm64/install/lib/libffmpeg.a \
  -headers build/ffmpeg/ios-iphoneos-arm64/install/include \
  -library build/ffmpeg/ios-iphonesimulator-arm64/install/lib/libffmpeg.a \
  -headers build/ffmpeg/ios-iphonesimulator-arm64/install/include \
  -output ffmpeg.xcframework