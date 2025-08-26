#!/bin/bash
ARCHS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

(
    cd repo/mbedtls
    python3 scripts/config.py set MBEDTLS_SSL_DTLS_SRTP
)

NDK_VERSION="27.1.12297006"

for ARCH in "${ARCHS[@]}"; do
    cmake -B build/mbedtls/android/$ARCH -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/${NDK_VERSION}/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=$ARCH \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_INSTALL_PREFIX=build/mbedtls/android/$ARCH/install \
        -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        repo/mbedtls
    cmake --build build/mbedtls/android/$ARCH --config Release --target install

    cmake -B build/libdatachannel/android/$ARCH -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/${NDK_VERSION}/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=$ARCH \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_INSTALL_PREFIX=build/libdatachannel/android/$ARCH/install \
        -DNO_TESTS=ON \
        -DNO_EXAMPLES=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_SHARED_DEPS_LIBS=OFF \
        -DENABLE_WARNINGS_AS_ERRORS=OFF \
        -DUSE_MBEDTLS=ON \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DMbedTLS_INCLUDE_DIR=build/mbedtls/android/$ARCH/install/include \
        -DMbedTLS_LIBRARY=build/mbedtls/android/$ARCH/install/lib/libmbedtls.a \
        -DMbedCrypto_LIBRARY=build/mbedtls/android/$ARCH/install/lib/libmbedcrypto.a \
        -DMbedX509_LIBRARY=build/mbedtls/android/$ARCH/install/lib/libmbedx509.a \
        repo/libdatachannel
    cmake --build build/libdatachannel/android/$ARCH --config Release --target install
done