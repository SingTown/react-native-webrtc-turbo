#import "Camera.h"
#import <AVFoundation/AVFoundation.h>
#import "MediaStreamTrack.h"

@interface Camera () <AVCaptureVideoDataOutputSampleBufferDelegate>
@property (nonatomic, strong) AVCaptureSession *session;
@property (nonatomic, strong) NSString *mediaStreamTrackId;
@property (nonatomic, strong) dispatch_queue_t sampleBufferQueue;
@end

@implementation Camera

- (instancetype)init:(NSString *)mediaStreamTrackId {
  self = [super init];
  if (self) {
    _session = [[AVCaptureSession alloc] init];
    _session.sessionPreset = AVCaptureSessionPreset1280x720;
    _sampleBufferQueue = dispatch_queue_create("com.example.camera.queue", DISPATCH_QUEUE_SERIAL);
    _mediaStreamTrackId = mediaStreamTrackId;
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

  CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
  int64_t pts_90k = (int64_t)round((double)pts.value * 90000.0 / pts.timescale);
  if (self.ptsBase == -1) {
    self.ptsBase = pts_90k;
  }
  CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

  size_t width = CVPixelBufferGetWidth(pixelBuffer);
  size_t height = CVPixelBufferGetHeight(pixelBuffer);

  auto frame = createAVFrame();
  frame->width = (int)width;
  frame->height = (int)height;
  frame->format = AV_PIX_FMT_NV12;
  frame->pts = pts_90k - self.ptsBase;

  int ret = av_frame_get_buffer(frame.get(), 32);
  if (ret < 0) {
      NSLog(@"av_frame_get_buffer failed: %d", ret);
      CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
      return;
  }
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


  std::string cppStr = [self.mediaStreamTrackId UTF8String];
  auto mediaStreamTrack = getMediaStreamTrack(cppStr);
  if (mediaStreamTrack) {
      mediaStreamTrack->push(frame);
  }
}

@end
