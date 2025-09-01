// Camera.m
#import "Camera.h"
#import <AVFoundation/AVFoundation.h>

@interface Camera () <AVCaptureVideoDataOutputSampleBufferDelegate>
@property (nonatomic, strong) AVCaptureSession *session;
@property (nonatomic, strong) NSString *ms;
@property (nonatomic, strong) dispatch_queue_t sampleBufferQueue;
@end

@implementation Camera

- (instancetype)init:(NSString *)ms {
  self = [super init];
  if (self) {
    _session = [[AVCaptureSession alloc] init];
    _session.sessionPreset = AVCaptureSessionPreset1280x720;
    _sampleBufferQueue = dispatch_queue_create("com.example.camera.queue", DISPATCH_QUEUE_SERIAL);
    _ms = ms;
    _ptsBase = -1;
    
    NSError *error = nil;
    if (![self setupCameraWithError:&error]) {
      NSLog(@"Camera setup failed: %@", error.localizedDescription);
      return nil;
    }
    
    NSLog(@"Camera init completed");
  }
  return self;
}

- (BOOL)setupCameraWithError:(NSError **)error {
  AVCaptureDevice *camera = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  if (!camera) {
    if (error) {
      *error = [NSError errorWithDomain:@"CameraError" code:1001 userInfo:@{NSLocalizedDescriptionKey: @"No camera device found"}];
    }
    return NO;
  }
  
  if ([camera lockForConfiguration:error]) {
    camera.activeVideoMinFrameDuration = CMTimeMake(1, 30); // 30fps
    camera.activeVideoMaxFrameDuration = CMTimeMake(1, 30);
    [camera unlockForConfiguration];
  }
  
  AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:camera error:error];
  if (!input) {
    return NO;
  }
  
  if ([_session canAddInput:input]) {
    [_session addInput:input];
  } else {
    if (error) {
      *error = [NSError errorWithDomain:@"CameraError" code:1002 userInfo:@{NSLocalizedDescriptionKey: @"Cannot add camera input"}];
    }
    return NO;
  }
  
  AVCaptureVideoDataOutput *output = [[AVCaptureVideoDataOutput alloc] init];
  output.videoSettings = @{ (id)kCVPixelBufferPixelFormatTypeKey :
                              @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) };
  [output setSampleBufferDelegate:self queue:_sampleBufferQueue];
  output.alwaysDiscardsLateVideoFrames = YES;
  
  if ([_session canAddOutput:output]) {
    [_session addOutput:output];
  } else {
    if (error) {
      *error = [NSError errorWithDomain:@"CameraError" code:1003 userInfo:@{NSLocalizedDescriptionKey: @"Cannot add camera output"}];
    }
    return NO;
  }
  
  return YES;
}

- (BOOL)isRunning {
  return self.session.isRunning;
}

- (BOOL)startCapture:(NSError **)error {
  if (!self.session.isRunning) {
    [self.session startRunning];
    return YES;
  }
  return YES;
}

- (void)stopCapture {
  if (self.session.isRunning) {
    [self.session stopRunning];
  }
}

#pragma mark - AVCaptureVideoDataOutputSampleBufferDelegate
- (void)captureOutput:(AVCaptureOutput *)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection {
}

@end
