#!/bin/bash
set -euo pipefail
ARCHS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

(
    cd repo/mbedtls
    python3 scripts/config.py set MBEDTLS_SSL_DTLS_SRTP
)

NDK_VERSION="27.1.12297006"

for ARCH in "${ARCHS[@]}"; do
    MBEDTLS_DIR="build/mbedtls/android/$ARCH"
    LIBDATACHANNEL_DIR="build/libdatachannel/android/$ARCH"
    OUTPUT_DIR="output/android/libdatachannel/$ARCH"
    cmake -B build/mbedtls/android/$ARCH -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/${NDK_VERSION}/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=$ARCH \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$MBEDTLS_DIR/install \
        -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        repo/mbedtls
    cmake --build build/mbedtls/android/$ARCH --config Release --target install

    cmake -B $LIBDATACHANNEL_DIR -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/${NDK_VERSION}/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=$ARCH \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_INSTALL_PREFIX=$LIBDATACHANNEL_DIR/install \
        -DCMAKE_BUILD_TYPE=Release \
        -DNO_WEBSOCKET=ON \
        -DNO_TESTS=ON \
        -DNO_EXAMPLES=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        -DENABLE_WARNINGS_AS_ERRORS=OFF \
        -DUSE_MBEDTLS=ON \
        -DUSE_GNUTLS=OFF \
        -DUSE_NICE=OFF \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DMbedTLS_INCLUDE_DIR=$MBEDTLS_DIR/install/include \
        -DMbedTLS_LIBRARY=$MBEDTLS_DIR/install/lib/libmbedtls.a \
        -DMbedCrypto_LIBRARY=$MBEDTLS_DIR/install/lib/libmbedcrypto.a \
        -DMbedX509_LIBRARY=$MBEDTLS_DIR/install/lib/libmbedx509.a \
        repo/libdatachannel
    cmake --build $LIBDATACHANNEL_DIR --config Release --target install

    rm -rf $OUTPUT_DIR
    mkdir -p $OUTPUT_DIR/lib
    cp -r $LIBDATACHANNEL_DIR/install/lib/*.so $OUTPUT_DIR/lib
    cp -r $LIBDATACHANNEL_DIR/install/include $OUTPUT_DIR/include
done