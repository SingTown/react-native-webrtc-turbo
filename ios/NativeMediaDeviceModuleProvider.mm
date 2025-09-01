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

- (void)createCamera:(NSString *)ms
             resolve:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject {
  
  // 检查摄像头权限
  AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
  
  if (authStatus == AVAuthorizationStatusDenied || authStatus == AVAuthorizationStatusRestricted) {
    reject(@"PERMISSION_DENIED", @"Camera permission is required", nil);
    return;
  }
  
  if (authStatus == AVAuthorizationStatusNotDetermined) {
    reject(@"PERMISSION_NOT_DETERMINED", @"Camera permission not determined", nil);
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

- (void)requestCameraPermission:(RCTPromiseResolveBlock)resolve
                         reject:(RCTPromiseRejectBlock)reject {
  
  AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
  
  if (authStatus == AVAuthorizationStatusAuthorized) {
    resolve(@YES);
    return;
  }
  
  if (authStatus == AVAuthorizationStatusRestricted) {
    resolve(@NO);
    return;
  }
  
  if (authStatus == AVAuthorizationStatusDenied) {
    // 权限已被拒绝，但在这个简化接口中仍然返回 false
    resolve(@NO);
    return;
  }
  
  // NotDetermined - 可以请求权限
  [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
    dispatch_async(dispatch_get_main_queue(), ^{
      resolve(@(granted));
    });
  }];
}

RCT_EXPORT_METHOD(createCamera:(NSString *)ms
                 resolver:(RCTPromiseResolveBlock)resolve
                 rejecter:(RCTPromiseRejectBlock)reject) {
  [self createCamera:ms resolve:resolve reject:reject];
}

RCT_EXPORT_METHOD(requestCameraPermission:(RCTPromiseResolveBlock)resolve
                                 rejecter:(RCTPromiseRejectBlock)reject) {
  [self requestCameraPermission:resolve reject:reject];
}

- (void)deleteCamera:(nonnull NSString *)ms {
  Camera *camera = [self.cameraMap objectForKey:ms];
  if (camera) {
    [camera stopCapture];
    [self.cameraMap removeObjectForKey:ms];
  }
}


- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
(const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeMediaDeviceSpecJSI>(params);
}


@end
