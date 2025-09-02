#import "WebrtcFabric.h"
#import "MediaStreamTrack.h"

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
  std::string _currentVideoStreamTrackId;
  std::string _currentAudioStreamTrackId;
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
  if (oldViewProps.videoStreamTrackId != newViewProps.videoStreamTrackId) {
    _currentVideoStreamTrackId = newViewProps.videoStreamTrackId;
  }
  if (oldViewProps.audioStreamTrackId != newViewProps.audioStreamTrackId) {
    _currentAudioStreamTrackId = newViewProps.audioStreamTrackId;
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
    if (self->_currentVideoStreamTrackId.empty()) {
      return;
    }

    auto mediaStream = getMediaStreamTrack(self->_currentVideoStreamTrackId);
    auto frame = mediaStream->pop(AV_PIX_FMT_RGBA);
    if (!frame) {
      return;
    }
   
    CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, frame->data[0], frame->linesize[0]*frame->height, NULL);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast;
    CGImageRef cgImage = CGImageCreate(
                                       frame->width,                          // width
                                       frame->height,                         // height
                                       8,                              // bitsPerComponent
                                       32,                             // bitsPerPixel (RGBA = 8*4)
                                       frame->linesize[0],                       // bytesPerRow
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
  });
}

@end
