#import <WebrtcSpec/WebrtcSpec.h>
#import <React/RCTBridgeModule.h>
#import "Camera.h"

@interface NativeMediaDeviceModuleProvider : NSObject <NativeMediaDeviceSpec, RCTBridgeModule>
@property (nonatomic, strong) NSMutableDictionary<NSString *, Camera *> *cameraMap;
@end
