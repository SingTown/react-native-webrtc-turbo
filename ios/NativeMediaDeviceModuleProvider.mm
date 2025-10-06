#import "NativeMediaDeviceModuleProvider.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <React/RCTUtils.h>

@implementation NativeMediaDeviceModuleProvider
RCT_EXPORT_MODULE()

- (void)requestPermission:(nonnull NSString *)name
                  resolve:(nonnull RCTPromiseResolveBlock)resolve
                   reject:(nonnull RCTPromiseRejectBlock)reject {
  AVMediaType permission;
  if ([name isEqualToString:@"camera"]) {
    permission = AVMediaTypeVideo;
  } else if ([name isEqualToString:@"microphone"]) {
    permission = AVMediaTypeAudio;
  } else {
    reject(@"INVALID_PERMISSION", [NSString stringWithFormat:@"Invalid permission name: %@", name], nil);
    return;
  }
  AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:permission];
  if (authStatus == AVAuthorizationStatusAuthorized) {
    resolve(@YES);
    return;
  } else if (authStatus == AVAuthorizationStatusNotDetermined) {
    [AVCaptureDevice requestAccessForMediaType:permission completionHandler:^(BOOL granted) {
      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(granted));
      });
    }];
  } else {
    resolve(@NO);
    return;
  }
}

- (void)cameraPush:(nonnull NSString *)container {
  CameraSession *cameraSession = [CameraSession sharedInstance];
  [cameraSession push:container];
}

- (void)cameraPop:(nonnull NSString *)container {
  CameraSession *cameraSession = [CameraSession sharedInstance];
  [cameraSession pop:container];
}

- (void)microphonePush:(nonnull NSString *)container { 
  AudioSession *audioSession = [AudioSession sharedInstance];
  [audioSession microphonePush:container];
}

- (void)microphonePop:(nonnull NSString *)container { 
  AudioSession *audioSession = [AudioSession sharedInstance];
  [audioSession microphonePop:container];
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
(const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeMediaDeviceSpecJSI>(params);
}


@end
