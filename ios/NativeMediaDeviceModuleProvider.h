#import <WebrtcSpec/WebrtcSpec.h>
#import <React/RCTBridgeModule.h>
#import "CameraSession.h"
#import "AudioSession.h"

@interface NativeMediaDeviceModuleProvider : NSObject <NativeMediaDeviceSpec, RCTBridgeModule>
@end
