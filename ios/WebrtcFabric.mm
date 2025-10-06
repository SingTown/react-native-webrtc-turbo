#import "WebrtcFabric.h"
#import "MediaContainer.h"
#import "AudioSession.h"

#import <react/renderer/components/WebrtcSpec/ComponentDescriptors.h>
#import <react/renderer/components/WebrtcSpec/EventEmitters.h>
#import <react/renderer/components/WebrtcSpec/Props.h>
#import <react/renderer/components/WebrtcSpec/RCTComponentViewHelpers.h>

#import "RCTFabricComponentsPlugins.h"

using namespace facebook::react;

@interface WebrtcFabric () <RCTWebrtcFabricViewProtocol>

@end

@implementation WebrtcFabric {
  UIView * _view;
  UIImageView * _imageView;
  CIContext * _ciContext;
  std::string _currentVideoContainer;
  std::string _currentAudioContainer;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider
{
  return concreteComponentDescriptorProvider<WebrtcFabricComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame:frame]) {
    static const auto defaultProps = std::make_shared<const WebrtcFabricProps>();
    _props = defaultProps;
    
    _view = [[UIView alloc] init];
    _imageView = [[UIImageView alloc] init];
    _imageView.contentMode = UIViewContentModeScaleAspectFit;
    [_view addSubview:_imageView];
    
    _ciContext = [CIContext contextWithOptions:nil];
    self.contentView = _view;
    [self scheduleNextVideoFrame];
  }
  
  return self;
}

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
  
  const auto &oldViewProps = *std::static_pointer_cast<WebrtcFabricProps const>(_props);
  const auto &newViewProps = *std::static_pointer_cast<WebrtcFabricProps const>(props);
  if (oldViewProps.videoContainer != newViewProps.videoContainer) {
    _currentVideoContainer = newViewProps.videoContainer;
  }
  if (oldViewProps.audioContainer != newViewProps.audioContainer) {
    _currentAudioContainer = newViewProps.audioContainer;
    AudioSession *audioSession = [AudioSession sharedInstance];
    [audioSession soundRemoveContainer:
     [NSString stringWithUTF8String:oldViewProps.audioContainer.c_str()]];
    [audioSession soundAddContainer:
     [NSString stringWithUTF8String:_currentAudioContainer.c_str()]];
  }
  
  [super updateProps:props oldProps:oldProps];
}

- (void)prepareForRecycle {
  AudioSession *audioSession = [AudioSession sharedInstance];
  [audioSession soundRemoveContainer:
   [NSString stringWithUTF8String:_currentAudioContainer.c_str()]];
  [super prepareForRecycle];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _imageView.frame = _view.bounds;
}

Class<RCTComponentViewProtocol> WebrtcFabricCls(void)
{
  return WebrtcFabric.class;
}

- (void)scheduleNextVideoFrame {
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_MSEC)), dispatch_get_main_queue(), ^{
    [self updateVideoFrame];
    [self scheduleNextVideoFrame];
  });
}

- (void)updateVideoFrame {
  if (self->_currentVideoContainer.empty()) {
    return;
  }
  
  auto container = getVideoContainer(self->_currentVideoContainer);
  if (!container) {
    return;
  }
  auto frame = container->popVideo(AV_PIX_FMT_RGB24);
  if (!frame) {
    return;
  }
  
  int rgbBufferSize = frame->width * frame->height * 3;
  uint8_t *rgbBuffer = (uint8_t *)malloc(rgbBufferSize);
  for (int y = 0; y < frame->height; y++) {
    uint8_t *src = frame->data[0] + y * frame->linesize[0];
    uint8_t *dst = rgbBuffer + y * frame->width * 3;
    memcpy(dst, src, frame->width * 3);
  }
  
  CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL,
                                                                rgbBuffer,
                                                                rgbBufferSize,
                                                                [](void *info,
                                                                   const void *data,
                                                                   size_t size) {
    free((void*)data);
  });
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaNone;
  CGImageRef cgImage = CGImageCreate(
                                     frame->width,                          // width
                                     frame->height,                         // height
                                     8,                              // bitsPerComponent
                                     24,                             // bitsPerPixel (RGBA= 8*3)
                                     frame->width * 3,                       // bytesPerRow
                                     colorSpace,                     // colorSpace
                                     bitmapInfo,                     // bitmapInfo
                                     dataProvider,                   // dataProvider
                                     NULL,                           // decode
                                     false,                          // shouldInterpolate
                                     kCGRenderingIntentDefault       // intent
                                     );
  
  if (cgImage) {
    CIImage *ciImage = [CIImage imageWithCGImage:cgImage];
    CGImageRef outputCGImage = [self->_ciContext createCGImage:ciImage fromRect:ciImage.extent];
    UIImage *uiImage = [UIImage imageWithCGImage:outputCGImage];
    
    dispatch_async(dispatch_get_main_queue(), ^{
      self->_imageView.image = uiImage;
      self->_imageView.frame = self->_view.bounds;
    });
    
    CGImageRelease(outputCGImage);
    CGImageRelease(cgImage);
  }
  
  CGColorSpaceRelease(colorSpace);
  CGDataProviderRelease(dataProvider);
}

@end
