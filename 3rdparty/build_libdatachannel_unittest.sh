#!/bin/bash
set -euo pipefail

(
    cd repo/mbedtls
    python3 scripts/config.py set MBEDTLS_SSL_DTLS_SRTP
)

MBEDTLS_DIR="build/mbedtls/unittest"
LIBDATACHANNEL_DIR="build/libdatachannel/unittest"
OUTPUT_DIR="output/unittest/libdatachannel"
cmake -B build/mbedtls/unittest \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$MBEDTLS_DIR/install \
    -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    repo/mbedtls
cmake --build build/mbedtls/unittest --config Release --target install

cmake -B $LIBDATACHANNEL_DIR \
    -DCMAKE_INSTALL_PREFIX=$LIBDATACHANNEL_DIR/install \
    -DCMAKE_BUILD_TYPE=Release \
    -DNO_WEBSOCKET=ON \
    -DNO_TESTS=ON \
    -DNO_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=ON \
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
mkdir -p $OUTPUT_DIR/include
cp -r $MBEDTLS_DIR/install/lib/*.a $OUTPUT_DIR/lib/
cp -r $MBEDTLS_DIR/install/include/* $OUTPUT_DIR/include/
cp -r $LIBDATACHANNEL_DIR/install/lib/*.a $OUTPUT_DIR/lib/
cp -r $LIBDATACHANNEL_DIR/install/include/* $OUTPUT_DIR/include/
