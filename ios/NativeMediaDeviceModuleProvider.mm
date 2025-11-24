#import "NativeMediaDeviceModuleProvider.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
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
		reject(@"INVALID_PERMISSION",
		       [NSString stringWithFormat:@"Invalid permission name: %@", name],
		       nil);
		return;
	}
	AVAuthorizationStatus authStatus =
	    [AVCaptureDevice authorizationStatusForMediaType:permission];
	if (authStatus == AVAuthorizationStatusAuthorized) {
		resolve(@YES);
		return;
	} else if (authStatus == AVAuthorizationStatusNotDetermined) {
		[AVCaptureDevice
		    requestAccessForMediaType:permission
		            completionHandler:^(BOOL granted) {
			          dispatch_async(dispatch_get_main_queue(), ^{
				        if (granted) {
					        resolve(@YES);
				        } else {
					        reject(@"NotAllowedError",
					               @"Permission denied by user", nil);
				        }
				        return;
			          });
		            }];
	} else {
		reject(@"NotAllowedError", @"Permission denied by user", nil);
		return;
	}
}

- (void)cameraAddPipe:(nonnull NSString *)pipeId {
	CameraSession *cameraSession = [CameraSession sharedInstance];
	[cameraSession addPipe:pipeId];
}

- (void)cameraRemovePipe:(nonnull NSString *)pipeId {
	CameraSession *cameraSession = [CameraSession sharedInstance];
	[cameraSession removePipe:pipeId];
}

- (void)microphoneAddPipe:(nonnull NSString *)pipeId {
	AudioSession *audioSession = [AudioSession sharedInstance];
	[audioSession microphoneAddPipe:pipeId];
}

- (void)microphoneRemovePipe:(nonnull NSString *)pipeId {
	AudioSession *audioSession = [AudioSession sharedInstance];
	[audioSession microphoneRemovePipe:pipeId];
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
	return std::make_shared<facebook::react::NativeMediaDeviceSpecJSI>(params);
}

@end
