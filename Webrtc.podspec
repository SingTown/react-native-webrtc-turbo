require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "Webrtc"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported }
  s.source       = { :git => "https://github.com/singtown/react-native-webrtc-turbo.git", :tag => "#{s.version}" }

  s.source_files = [
    "ios/**/*.{h,m,mm,cpp}",
    "cpp/NativeDatachannelModule.h",
    "cpp/bridgeType.h",
    "cpp/guid.h",
    "cpp/ffmpeg.h",
    "cpp/NativeDatachannelModule.cpp",
    "cpp/Decoder.cpp",
  ]
  s.private_header_files = "ios/**/*.h", "cpp/**/*.h"

  # Build libdatachannel xcframework if it doesn't exist
  s.prepare_command = <<-CMD
    if [ ! -d "3rdparty/ffmpeg.xcframework" ]; then
      echo "Building ffmpeg xcframework..."
      cd 3rdparty && bash build_ffmpeg_ios.sh
    fi
    if [ ! -d "3rdparty/libdatachannel.xcframework" ]; then
      echo "Building libdatachannel xcframework..."
      cd 3rdparty && bash build_libdatachannel_ios.sh
    fi
  CMD

  s.vendored_frameworks = "3rdparty/*.xcframework"

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => [
      '$(PODS_TARGET_SRCROOT)/3rdparty/ffmpeg.xcframework/ios-arm64/Headers',
      '$(PODS_TARGET_SRCROOT)/3rdparty/libdatachannel.xcframework/ios-arm64/Headers'
    ].join(' ')
  }

  install_modules_dependencies(s)
end
