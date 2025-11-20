#import "WebrtcFabric.h"
#import "AudioSession.h"
#import "framepipe.h"

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
  std::string _currentVideoPipeId;
  int _lastCallbackId;
  std::string _currentAudioPipeId;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider
{
  return concreteComponentDescriptorProvider<WebrtcFabricComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame:frame]) {
    _lastCallbackId = -1;
    
    static const auto defaultProps = std::make_shared<const WebrtcFabricProps>();
    _props = defaultProps;
    
    _view = [[UIView alloc] init];
    _imageView = [[UIImageView alloc] init];
    _imageView.contentMode = UIViewContentModeScaleAspectFit;
    [_view addSubview:_imageView];
    
    _ciContext = [CIContext contextWithOptions:nil];
    self.contentView = _view;
  }
  
  return self;
}

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
  
  const auto &oldViewProps = *std::static_pointer_cast<WebrtcFabricProps const>(_props);
  const auto &newViewProps = *std::static_pointer_cast<WebrtcFabricProps const>(props);
  if (oldViewProps.videoPipeId != newViewProps.videoPipeId) {
    _currentVideoPipeId = newViewProps.videoPipeId;
    
    if (_lastCallbackId > 0) {
      unsubscribe(_lastCallbackId);
    }
    auto scaler = std::make_shared<Scaler>();
    subscribe(_currentVideoPipeId, [self, scaler](std::shared_ptr<AVFrame> frame) {
      auto scaledFrame = scaler->scale(frame, AV_PIX_FMT_RGB24, frame->width, frame->height);
      [self updateVideoFrame: scaledFrame];
    });
  }
  if (oldViewProps.audioPipeId != newViewProps.audioPipeId) {
    _currentAudioPipeId = newViewProps.audioPipeId;
    AudioSession *audioSession = [AudioSession sharedInstance];
    [audioSession soundRemovePipe:
     [NSString stringWithUTF8String:oldViewProps.audioPipeId.c_str()]];
    [audioSession soundAddPipe:
     [NSString stringWithUTF8String:_currentAudioPipeId.c_str()]];
  }
  
  [super updateProps:props oldProps:oldProps];
}

- (void)prepareForRecycle {
  AudioSession *audioSession = [AudioSession sharedInstance];
  unsubscribe(_lastCallbackId);
  [audioSession soundRemovePipe:
   [NSString stringWithUTF8String:_currentAudioPipeId.c_str()]];
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

- (void)updateVideoFrame: (std::shared_ptr<AVFrame> )frame {
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
