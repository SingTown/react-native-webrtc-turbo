ARCHS=("aarch64" "armv7" "x86" "x86_64")
CPUS=("armv8-a" "armv7-a" "i686" "x86-64")
TOOLCHAIN_ARCHS=("aarch64-linux-android" "armv7a-linux-androideabi" "i686-linux-android" "x86_64-linux-android")

for i in "${!ARCHS[@]}"; do
    ARCH="${ARCHS[$i]}"
    CPU="${CPUS[$i]}"
    TOOLCHAIN_ARCH="${TOOLCHAIN_ARCHS[$i]}"

    (
        mkdir -p build/ffmpeg/android-$ARCH
        cd build/ffmpeg/android-$ARCH
        ../../../ffmpeg/configure \
            --host-os=darwin-x86_64 --target-os=android \
            --enable-cross-compile --arch=$ARCH --cpu=$CPU \
            --sysroot=$ANDROID_HOME/ndk/27.1.12297006/toolchains/llvm/prebuilt/darwin-x86_64/sysroot \
            --sysinclude=$ANDROID_HOME/ndk/27.1.12297006/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/ \
            --cc=$ANDROID_HOME/ndk/27.1.12297006/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-clang \
            --cxx=$ANDROID_HOME/ndk/27.1.12297006/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-clang++ \
            --strip=$ANDROID_HOME/ndk/27.1.12297006/toolchains/llvm/prebuilt/darwin-x86_64/bin/${TOOLCHAIN_ARCH}24-strip \
            --prefix=install \
            --disable-everything \
            --disable-shared --enable-static \
            --enable-jni --enable-mediacodec \
            --disable-avformat \
            --disable-avdevice \
            --disable-avfilter \
            --disable-swscale \
            --disable-swresample \
            --enable-avcodec \
            --enable-avutil \
            --enable-decoder=h264 \
            --enable-decoder=hevc \
            --enable-parser=h264 \
            --enable-parser=hevc

        make -j install
    )
done
