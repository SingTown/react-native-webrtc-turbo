# build MBEDTLS
(
    cd mbedtls
    python3 scripts/config.py set MBEDTLS_SSL_DTLS_SRTP
)

cmake -B build/mbedtls/ios-arm64 -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT=iphoneos \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_INSTALL_PREFIX=build/mbedtls/ios-arm64/install \
    -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    mbedtls
cmake --build build/mbedtls/ios-arm64 --config Release --target install

cmake -B build/mbedtls/ios-sim-arm64 -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT=iphonesimulator \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_INSTALL_PREFIX=build/mbedtls/ios-sim-arm64/install \
    -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    mbedtls
cmake --build build/mbedtls/ios-sim-arm64 --config Release --target install

# build libdatachannel
cmake -B build/libdatachannel/ios-arm64 -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT=iphoneos \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_INSTALL_PREFIX=build/libdatachannel/ios-arm64/install \
    -DNO_TESTS=ON \
    -DNO_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_SHARED_DEPS_LIBS=OFF \
    -DENABLE_WARNINGS_AS_ERRORS=OFF \
    -DUSE_MBEDTLS=ON \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
    -DMbedTLS_INCLUDE_DIR=build/mbedtls/ios-arm64/install/include \
    -DMbedTLS_LIBRARY=build/mbedtls/ios-arm64/install/lib/libmbedtls.a \
    -DMbedCrypto_LIBRARY=build/mbedtls/ios-arm64/install/lib/libmbedcrypto.a \
    -DMbedX509_LIBRARY=build/mbedtls/ios-arm64/install/lib/libmbedx509.a \
    libdatachannel
cmake --build build/libdatachannel/ios-arm64 --config Release --target install

cmake -B build/libdatachannel/ios-sim-arm64 -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT=iphonesimulator \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_INSTALL_PREFIX=build/libdatachannel/ios-sim-arm64/install \
    -DNO_TESTS=ON \
    -DNO_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_SHARED_DEPS_LIBS=OFF \
    -DENABLE_WARNINGS_AS_ERRORS=OFF \
    -DUSE_MBEDTLS=ON \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
    -DMbedTLS_INCLUDE_DIR=build/mbedtls/ios-sim-arm64/install/include \
    -DMbedTLS_LIBRARY=build/mbedtls/ios-sim-arm64/install/lib/libmbedtls.a \
    -DMbedCrypto_LIBRARY=build/mbedtls/ios-sim-arm64/install/lib/libmbedcrypto.a \
    -DMbedX509_LIBRARY=build/mbedtls/ios-sim-arm64/install/lib/libmbedx509.a \
    libdatachannel
cmake --build build/libdatachannel/ios-sim-arm64 --config Release --target install

# 合并静态库
libtool -static \
  build/mbedtls/ios-arm64/install/lib/libmbedcrypto.a \
  build/mbedtls/ios-arm64/install/lib/libmbedx509.a \
  build/mbedtls/ios-arm64/install/lib/libmbedtls.a \
  build/libdatachannel/ios-arm64/install/lib/libdatachannel.a \
  build/libdatachannel/ios-arm64/install/lib/libjuice.a \
  build/libdatachannel/ios-arm64/install/lib/libsrtp2.a \
  build/libdatachannel/ios-arm64/install/lib/libusrsctp.a \
  -o build/libdatachannel/ios-arm64/libdatachannel.a

libtool -static \
  build/mbedtls/ios-sim-arm64/install/lib/libmbedcrypto.a \
  build/mbedtls/ios-sim-arm64/install/lib/libmbedx509.a \
  build/mbedtls/ios-sim-arm64/install/lib/libmbedtls.a \
  build/libdatachannel/ios-sim-arm64/install/lib/libdatachannel.a \
  build/libdatachannel/ios-sim-arm64/install/lib/libjuice.a \
  build/libdatachannel/ios-sim-arm64/install/lib/libsrtp2.a \
  build/libdatachannel/ios-sim-arm64/install/lib/libusrsctp.a \
  -o build/libdatachannel/ios-sim-arm64/libdatachannel.a

rm -rf libdatachannel.xcframework
xcodebuild -create-xcframework \
  -library build/libdatachannel/ios-arm64/libdatachannel.a \
  -headers build/libdatachannel/ios-arm64/install/include \
  -library build/libdatachannel/ios-sim-arm64/libdatachannel.a \
  -headers build/libdatachannel/ios-sim-arm64/install/include \
  -output libdatachannel.xcframework