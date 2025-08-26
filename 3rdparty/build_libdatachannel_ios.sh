SDKS=("iphoneos" "iphonesimulator" "iphonesimulator")
ARCHS=("arm64" "arm64" "x86_64")

(
    cd mbedtls
    python3 scripts/config.py set MBEDTLS_SSL_DTLS_SRTP
)

for i in "${!SDKS[@]}"; do
    SDK="${SDKS[$i]}"
    ARCH="${ARCHS[$i]}"

    MBEDTLS_DIR="build/mbedtls/$SDK/$ARCH"
    cmake -B $MBEDTLS_DIR -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_ARCHITECTURES=$ARCH \
        -DCMAKE_OSX_SYSROOT=$SDK \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=15.1 \
        -DCMAKE_INSTALL_PREFIX=$MBEDTLS_DIR/install \
        -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        mbedtls
    cmake --build $MBEDTLS_DIR --config Release --target install

    LIBDATACHANNEL_DIR="build/libdatachannel/$SDK/$ARCH"
    cmake -B $LIBDATACHANNEL_DIR -G Xcode \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_ARCHITECTURES=$ARCH \
        -DCMAKE_OSX_SYSROOT=$SDK \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=15.1 \
        -DCMAKE_INSTALL_PREFIX=$LIBDATACHANNEL_DIR/install \
        -DNO_TESTS=ON \
        -DNO_EXAMPLES=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_SHARED_DEPS_LIBS=OFF \
        -DENABLE_WARNINGS_AS_ERRORS=OFF \
        -DUSE_MBEDTLS=ON \
        -DMbedTLS_INCLUDE_DIR=$MBEDTLS_DIR/install/include \
        -DMbedTLS_LIBRARY=$MBEDTLS_DIR/install/lib/libmbedtls.a \
        -DMbedCrypto_LIBRARY=$MBEDTLS_DIR/install/lib/libmbedcrypto.a \
        -DMbedX509_LIBRARY=$MBEDTLS_DIR/install/lib/libmbedx509.a \
        libdatachannel
    cmake --build $LIBDATACHANNEL_DIR --config Release --target install

    libtool -static \
        $MBEDTLS_DIR/install/lib/libmbedcrypto.a \
        $MBEDTLS_DIR/install/lib/libmbedx509.a \
        $MBEDTLS_DIR/install/lib/libmbedtls.a \
        $LIBDATACHANNEL_DIR/install/lib/libdatachannel.a \
        $LIBDATACHANNEL_DIR/install/lib/libjuice.a \
        $LIBDATACHANNEL_DIR/install/lib/libsrtp2.a \
        $LIBDATACHANNEL_DIR/install/lib/libusrsctp.a \
        -o $LIBDATACHANNEL_DIR/libdatachannel.a
done

libtool -static \
    build/libdatachannel/iphoneos/arm64/libdatachannel.a \
    -o build/libdatachannel/iphoneos/libdatachannel.a

libtool -static \
    build/libdatachannel/iphonesimulator/arm64/libdatachannel.a \
    build/libdatachannel/iphonesimulator/x86_64/libdatachannel.a \
    -o build/libdatachannel/iphonesimulator/libdatachannel.a

rm -rf libdatachannel.xcframework
xcodebuild -create-xcframework \
    -library build/libdatachannel/iphoneos/libdatachannel.a \
    -headers build/libdatachannel/iphoneos/arm64/install/include \
    -library build/libdatachannel/iphonesimulator/libdatachannel.a \
    -headers build/libdatachannel/iphonesimulator/arm64/install/include \
    -output libdatachannel.xcframework