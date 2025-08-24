#import "WebrtcView.h"
#import "NativeDatachannelModule.h"

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
  std::string _currentVideoTrackId;
  std::string _currentAudioTrackId;
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
    CADisplayLink * _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateFrame)];
    _displayLink.preferredFramesPerSecond = 100;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
  
  return self;
}

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
  
  const auto &oldViewProps = *std::static_pointer_cast<WebrtcFabricProps const>(_props);
  const auto &newViewProps = *std::static_pointer_cast<WebrtcFabricProps const>(props);
  if (oldViewProps.videoTrackId != newViewProps.videoTrackId) {
    _currentVideoTrackId = newViewProps.videoTrackId;
  }
  if (oldViewProps.audioTrackId != newViewProps.audioTrackId) {
    _currentAudioTrackId = newViewProps.audioTrackId;
  }
  
  [super updateProps:props oldProps:oldProps];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _imageView.frame = _view.bounds;
}

Class<RCTComponentViewProtocol> WebrtcFabricCls(void)
{
  return WebrtcFabric.class;
}

- (void)updateFrame {
  
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
    std::optional<ArgbFrame> argbFrame = getTrackBuffer(self->_currentVideoTrackId);
    if (!argbFrame.has_value()) {
      return;
    }
    NSData* resultData = [NSData dataWithBytes:argbFrame->data.data() length:argbFrame->data.size()];
    [self displayImageFromRGBData:resultData width:argbFrame->width height:argbFrame->height linesize:argbFrame->linesize];
  });
}

- (void)displayImageFromRGBData:(NSData *)data width:(int)width height:(int)height linesize:(int)linesize {
  if (!data || data.length == 0) {
    return;
  }
  
  CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, data.bytes, data.length, NULL);
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGBitmapInfo bitmapInfo = kCGImageAlphaPremultipliedFirst;
  
  
  CGImageRef cgImage = CGImageCreate(
                                     width,                          // width
                                     height,                         // height
                                     8,                              // bitsPerComponent
                                     32,                             // bitsPerPixel (ARGB = 8*4)
                                     linesize,                       // bytesPerRow
                                     colorSpace,                     // colorSpace
                                     bitmapInfo,                     // bitmapInfo
                                     dataProvider,                   // dataProvider
                                     NULL,                           // decode
                                     false,                          // shouldInterpolate
                                     kCGRenderingIntentDefault       // intent
                                     );
  
  if (cgImage) {
    CIImage *ciImage = [CIImage imageWithCGImage:cgImage];
    CGImageRef outputCGImage = [_ciContext createCGImage:ciImage fromRect:ciImage.extent];
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
