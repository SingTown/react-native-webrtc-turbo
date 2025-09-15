#import "NativeMediaDeviceModuleProvider.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <React/RCTUtils.h>

@implementation NativeMediaDeviceModuleProvider
RCT_EXPORT_MODULE()

- (instancetype)init {
  self = [super init];
  if (self) {
    self.cameraMap = [NSMutableDictionary dictionary];
  }
  return self;
}

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

- (void)createCamera:(NSString *)ms
             resolve:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject {
  
  AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
  if (authStatus != AVAuthorizationStatusAuthorized) {
    reject(@"PERMISSION_DENIED", @"Camera permission is required", nil);
    return;
  }
  
  @try {
    Camera *camera = [[Camera alloc] init:ms];
    if (!camera) {
      reject(@"CAMERA_INIT_FAILED", @"Failed to initialize camera", nil);
      return;
    }
    
    NSError *error = nil;
    if (![camera startCapture:&error]) {
      reject(@"CAMERA_START_FAILED", error.localizedDescription ?: @"Failed to start camera capture", error);
      return;
    }
    
    [self.cameraMap setObject:camera forKey:ms];
    resolve(@"Camera opened successfully");
  }
  @catch (NSException *exception) {
    reject(@"CAMERA_OPEN_FAILED", exception.reason, nil);
  }
}

RCT_EXPORT_METHOD(createCamera:(NSString *)ms
                  resolver:(RCTPromiseResolveBlock)resolve
                  rejecter:(RCTPromiseRejectBlock)reject) {
  [self createCamera:ms resolve:resolve reject:reject];
}

- (void)deleteCamera:(nonnull NSString *)ms {
  Camera *camera = [self.cameraMap objectForKey:ms];
  if (camera) {
    [camera stopCapture];
    [self.cameraMap removeObjectForKey:ms];
  }
}

- (void)createAudio:(NSString *)ms
            resolve:(RCTPromiseResolveBlock)resolve
             reject:(RCTPromiseRejectBlock)reject {
  
  AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
  if (authStatus != AVAuthorizationStatusAuthorized) {
    reject(@"PERMISSION_DENIED", @"Microphone permission is required", nil);
    return;
  }
  
  @try {
    AudioSession *audioSession = [AudioSession sharedInstance];
    [audioSession capturePushMediaStreamTrack:ms];
    
    resolve(@"Audio capture started successfully");
  }
  @catch (NSException *exception) {
    reject(@"AUDIO_OPEN_FAILED", exception.reason, nil);
  }
}

RCT_EXPORT_METHOD(createAudio:(NSString *)ms
                  resolver:(RCTPromiseResolveBlock)resolve
                  rejecter:(RCTPromiseRejectBlock)reject) {
  [self createAudio:ms resolve:resolve reject:reject];
}

- (void)deleteAudio:(nonnull NSString *)ms {
  AudioSession *audioSession = [AudioSession sharedInstance];
  [audioSession capturePopMediaStreamTrack:ms];
}


- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
(const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeMediaDeviceSpecJSI>(params);
}


@end
