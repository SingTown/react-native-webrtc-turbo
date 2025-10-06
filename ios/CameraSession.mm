#import "CameraSession.h"
#import <AVFoundation/AVFoundation.h>
#import "MediaContainer.h"

static CameraSession *_sharedInstance = nil;

@interface CameraSession () <AVCaptureVideoDataOutputSampleBufferDelegate>
@property (nonatomic, strong) AVCaptureSession *session;
@property (nonatomic, strong) dispatch_queue_t sampleBufferQueue;
@property (nonatomic, strong) NSMutableArray<NSString *> *containers;
@property (nonatomic, assign) int64_t ptsBase;

@end

@implementation CameraSession

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _sharedInstance = [[CameraSession alloc] init];
  });
  return _sharedInstance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    self.session = [[AVCaptureSession alloc] init];
    self.session.sessionPreset = AVCaptureSessionPreset1280x720;
    self.sampleBufferQueue = dispatch_queue_create("com.example.camera.queue", DISPATCH_QUEUE_SERIAL);
    self.containers = [NSMutableArray array];
    self.ptsBase = -1;
    
    NSError *error = nil;
    if (![self setupCameraWithError:&error]) {
      NSLog(@"Camera setup failed: %@", error.localizedDescription);
      return nil;
    }
    
    NSLog(@"Camera init completed");
  }
  return self;
}

- (void)addContainer:(NSString *)container {
  if ([self.containers containsObject:container]) {
    return;
  }
  [self.containers addObject:container];
  
  if (!self.session.isRunning) {
    [self.session startRunning];
  }
}

- (void)removeContainer:(NSString *)container {
  if (![self.containers containsObject:container]) {
    return;
  }
  [self.containers removeObject:container];
  if (self.containers.count == 0) {
    if (self.session.isRunning) {
      [self.session stopRunning];
    }
  }
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

#pragma mark - AVCaptureVideoDataOutputSampleBufferDelegate
- (void)captureOutput:(AVCaptureOutput *)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection {
  
  CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
  int64_t pts_90k = (int64_t)round((double)pts.value * 90000.0 / pts.timescale);
  if (self.ptsBase == -1) {
    self.ptsBase = pts_90k;
  }
  CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
  
  size_t width = CVPixelBufferGetWidth(pixelBuffer);
  size_t height = CVPixelBufferGetHeight(pixelBuffer);
  
  auto frame = createVideoFrame(AV_PIX_FMT_NV12, pts_90k - self.ptsBase, width, height);
  uint8_t *srcY = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
  uint8_t *srcUV = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
  int srcYStride = (int)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
  int srcUVStride = (int)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);
  
  // Copy Y
  for (size_t i = 0; i < height; ++i) {
    memcpy(frame->data[0] + i * frame->linesize[0], srcY + i * srcYStride, width);
  }
  // Copy UV
  for (size_t i = 0; i < height / 2; ++i) {
    memcpy(frame->data[1] + i * frame->linesize[1], srcUV + i * srcUVStride, width);
  }
  
  CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
  
  for (NSString *containerId in self.containers) {
    std::string cppStr = [containerId UTF8String];
    auto container = getMediaContainer(cppStr);
    if (container) {
      container->push(frame);
    }
  }
}

@end
