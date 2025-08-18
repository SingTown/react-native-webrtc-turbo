#import "WebrtcView.h"
#import "NativeDatachannelModule.h"

#import <react/renderer/components/WebrtcSpec/ComponentDescriptors.h>
#import <react/renderer/components/WebrtcSpec/EventEmitters.h>
#import <react/renderer/components/WebrtcSpec/Props.h>
#import <react/renderer/components/WebrtcSpec/RCTComponentViewHelpers.h>

#import "RCTFabricComponentsPlugins.h"

using namespace facebook::react;

@interface WebrtcView () <RCTWebrtcViewViewProtocol>

@end

@implementation WebrtcView {
  UIView * _view;
  UIImageView * _imageView;
  CIContext * _ciContext;
  int _currentTrackId;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider
{
  return concreteComponentDescriptorProvider<WebrtcViewComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame:frame]) {
    static const auto defaultProps = std::make_shared<const WebrtcViewProps>();
    _props = defaultProps;
    
    _view = [[UIView alloc] init];
    _imageView = [[UIImageView alloc] init];
    _imageView.contentMode = UIViewContentModeScaleAspectFit;
    [_view addSubview:_imageView];
    
    _ciContext = [CIContext contextWithOptions:nil];
    self.contentView = _view;
    CADisplayLink * _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateFrame)];
    _displayLink.preferredFramesPerSecond = 30;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
  
  return self;
}

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
  
  const auto &oldViewProps = *std::static_pointer_cast<WebrtcViewProps const>(_props);
  const auto &newViewProps = *std::static_pointer_cast<WebrtcViewProps const>(props);
  if (oldViewProps.trackId != newViewProps.trackId) {
    _currentTrackId = newViewProps.trackId;
    
  }
  
  [super updateProps:props oldProps:oldProps];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _imageView.frame = _view.bounds;
}

Class<RCTComponentViewProtocol> WebrtcViewCls(void)
{
  return WebrtcView.class;
}

- (void)updateFrame {
  
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
    std::vector<uint8_t> buffer = getTrackBuffer(self->_currentTrackId);
    
    if (!buffer.empty()) {
      NSData* resultData = [NSData dataWithBytes:buffer.data() length:buffer.size()];
      [self displayImageFromRGBData:resultData width:720 height:480];
    }
  });
}

- (void)displayImageFromRGBData:(NSData *)data width:(int)width height:(int)height {
  if (!data || data.length == 0) {
    return;
  }
  
  CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, data.bytes, data.length, NULL);
  
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  
  CGImageRef cgImage = CGImageCreate(
                                     width,                          // width
                                     height,                         // height
                                     8,                              // bitsPerComponent
                                     24,                             // bitsPerPixel (RGB = 8*3)
                                     width * 3,                      // bytesPerRow
                                     colorSpace,                     // colorSpace
                                     kCGBitmapByteOrderDefault,      // bitmapInfo
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
