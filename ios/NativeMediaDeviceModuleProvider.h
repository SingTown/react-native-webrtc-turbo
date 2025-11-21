#import "AudioSession.h"
#import "CameraSession.h"
#import <React/RCTBridgeModule.h>
#import <WebrtcSpec/WebrtcSpec.h>

@interface NativeMediaDeviceModuleProvider
    : NSObject <NativeMediaDeviceSpec, RCTBridgeModule>
@end
