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

  s.source_files = "ios/**/*.{h,m,mm,cpp}", "cpp/*.{h,m,mm,cpp}", "cpp/jsi/*.{h,m,mm,cpp}"
  s.private_header_files = "ios/**/*.h", "cpp/*.h", "cpp/jsi/*.h"
  s.vendored_frameworks = "3rdparty/output/ios/*.xcframework"
  s.frameworks = 'AVFoundation', 'CoreMedia', 'CoreVideo', 'VideoToolbox'

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => [
      '$(PODS_TARGET_SRCROOT)/3rdparty/output/ios/ffmpeg.xcframework/ios-arm64/Headers',
      '$(PODS_TARGET_SRCROOT)/3rdparty/output/ios/libdatachannel.xcframework/ios-arm64/Headers',
    ].join(' ')
  }

  install_modules_dependencies(s)
end
