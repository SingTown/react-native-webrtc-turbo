#import "WebrtcFabric.h"
#import "AudioSession.h"
#import "framepipe.h"

#import <react/renderer/components/WebrtcSpec/ComponentDescriptors.h>
#import <react/renderer/components/WebrtcSpec/EventEmitters.h>
#import <react/renderer/components/WebrtcSpec/Props.h>
#import <react/renderer/components/WebrtcSpec/RCTComponentViewHelpers.h>

#import "RCTFabricComponentsPlugins.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

using namespace facebook::react;

@interface WebrtcFabric () <RCTWebrtcFabricViewProtocol>

@end

@implementation WebrtcFabric {
	UIView *_view;
	AVSampleBufferDisplayLayer *_displayLayer;
	std::string _currentVideoPipeId;
	int _lastCallbackId;
	std::string _currentAudioPipeId;
	CMVideoFormatDescriptionRef _formatDescription;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider {
	return concreteComponentDescriptorProvider<
	    WebrtcFabricComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame {
	if (self = [super initWithFrame:frame]) {
		_lastCallbackId = -1;
		_formatDescription = NULL;

		static const auto defaultProps =
		    std::make_shared<const WebrtcFabricProps>();
		_props = defaultProps;

		_view = [[UIView alloc] init];

		// 创建 AVSampleBufferDisplayLayer
		_displayLayer = [[AVSampleBufferDisplayLayer alloc] init];
		_displayLayer.videoGravity = AVLayerVideoGravityResizeAspect;
		_displayLayer.backgroundColor = [UIColor blackColor].CGColor;
		[_view.layer addSublayer:_displayLayer];

		self.contentView = _view;
	}

	return self;
}

- (void)updateProps:(Props::Shared const &)props
           oldProps:(Props::Shared const &)oldProps {

	const auto &oldViewProps =
	    *std::static_pointer_cast<WebrtcFabricProps const>(_props);
	const auto &newViewProps =
	    *std::static_pointer_cast<WebrtcFabricProps const>(props);
	if (oldViewProps.videoPipeId != newViewProps.videoPipeId) {
		_currentVideoPipeId = newViewProps.videoPipeId;

		if (_lastCallbackId > 0) {
			unsubscribe(_lastCallbackId);
		}
		auto scaler = std::make_shared<Scaler>();
		subscribe(
		    {_currentVideoPipeId},
		    [self, scaler](std::string, int, std::shared_ptr<AVFrame> frame) {
			    auto scaledFrame = scaler->scale(frame, AV_PIX_FMT_NV12,
			                                     frame->width, frame->height);
			    [self updateVideoFrame:scaledFrame];
		    });
	}
	if (oldViewProps.audioPipeId != newViewProps.audioPipeId) {
		_currentAudioPipeId = newViewProps.audioPipeId;
		AudioSession *audioSession = [AudioSession sharedInstance];
		[audioSession
		    soundRemovePipe:[NSString
		                        stringWithUTF8String:oldViewProps.audioPipeId
		                                                 .c_str()]];
		[audioSession
		    soundAddPipe:[NSString
		                     stringWithUTF8String:_currentAudioPipeId.c_str()]];
	}
	if (oldViewProps.resizeMode != newViewProps.resizeMode) {
		if (newViewProps.resizeMode == "cover") {
			_displayLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
		} else if (newViewProps.resizeMode == "contain") {
			_displayLayer.videoGravity = AVLayerVideoGravityResizeAspect;
		} else if (newViewProps.resizeMode == "fill") {
			_displayLayer.videoGravity = AVLayerVideoGravityResize;
		}
	}

	[super updateProps:props oldProps:oldProps];
}

- (void)prepareForRecycle {
	AudioSession *audioSession = [AudioSession sharedInstance];
	unsubscribe(_lastCallbackId);
	[audioSession
	    soundRemovePipe:[NSString
	                        stringWithUTF8String:_currentAudioPipeId.c_str()]];

	if (_formatDescription) {
		CFRelease(_formatDescription);
		_formatDescription = NULL;
	}
	[_displayLayer flushAndRemoveImage];

	[super prepareForRecycle];
}

- (void)dealloc {
	if (_formatDescription) {
		CFRelease(_formatDescription);
	}
}

- (void)layoutSubviews {
	[super layoutSubviews];
	_displayLayer.frame = _view.bounds;
}

Class<RCTComponentViewProtocol> WebrtcFabricCls(void) {
	return WebrtcFabric.class;
}

- (void)updateVideoFrame:(std::shared_ptr<AVFrame>)frame {
	if (!frame) {
		return;
	}

	int width = frame->width;
	int height = frame->height;

	if (_formatDescription == NULL ||
	    CMVideoFormatDescriptionGetDimensions(_formatDescription).width !=
	        width ||
	    CMVideoFormatDescriptionGetDimensions(_formatDescription).height !=
	        height) {

		if (_formatDescription) {
			CFRelease(_formatDescription);
		}

		OSStatus status = CMVideoFormatDescriptionCreate(
		    kCFAllocatorDefault,
		    kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, // NV12
		    width, height, NULL, &_formatDescription);

		if (status != noErr) {
			NSLog(@"Failed to create format description: %d", status);
			return;
		}
	}

	CVPixelBufferRef pixelBuffer = NULL;
	NSDictionary *pixelBufferAttributes = @{
		(NSString *)kCVPixelBufferPixelFormatTypeKey :
		    @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
		(NSString *)kCVPixelBufferWidthKey : @(width),
		(NSString *)kCVPixelBufferHeightKey : @(height),
		(NSString *)kCVPixelBufferIOSurfacePropertiesKey : @{}
	};

	CVReturn result = CVPixelBufferCreate(
	    kCFAllocatorDefault, width, height,
	    kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
	    (__bridge CFDictionaryRef)pixelBufferAttributes, &pixelBuffer);

	if (result != kCVReturnSuccess) {
		NSLog(@"Failed to create pixel buffer: %d", result);
		return;
	}

	CVPixelBufferLockBaseAddress(pixelBuffer, 0);

	// COPY Y PLANE
	uint8_t *yDestPlane =
	    (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
	size_t yDestBytesPerRow =
	    CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
	uint8_t *ySrcPlane = frame->data[0];
	size_t ySrcBytesPerRow = frame->linesize[0];

	for (int row = 0; row < height; row++) {
		memcpy(yDestPlane + row * yDestBytesPerRow,
		       ySrcPlane + row * ySrcBytesPerRow, width);
	}

	// COPY UV PLANE
	uint8_t *uvDestPlane =
	    (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
	size_t uvDestBytesPerRow =
	    CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);
	uint8_t *uvSrcPlane = frame->data[1];
	size_t uvSrcBytesPerRow = frame->linesize[1];

	for (int row = 0; row < height / 2; row++) {
		memcpy(uvDestPlane + row * uvDestBytesPerRow,
		       uvSrcPlane + row * uvSrcBytesPerRow, width);
	}

	CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

	CMSampleBufferRef sampleBuffer = NULL;
	CMSampleTimingInfo timingInfo = {
	    .duration = kCMTimeInvalid,
	    .presentationTimeStamp =
	        CMTimeMakeWithSeconds(CACurrentMediaTime(), 1000000000),
	    .decodeTimeStamp = kCMTimeInvalid};

	OSStatus status = CMSampleBufferCreateReadyWithImageBuffer(
	    kCFAllocatorDefault, pixelBuffer, _formatDescription, &timingInfo,
	    &sampleBuffer);

	CVPixelBufferRelease(pixelBuffer);

	if (status != noErr) {
		NSLog(@"Failed to create sample buffer: %d", status);
		return;
	}

	dispatch_async(dispatch_get_main_queue(), ^{
	  if (self->_displayLayer.status ==
		  AVQueuedSampleBufferRenderingStatusFailed) {
		  [self->_displayLayer flush];
	  }

	  if (self->_displayLayer.isReadyForMoreMediaData) {
		  [self->_displayLayer enqueueSampleBuffer:sampleBuffer];
	  }

	  CFRelease(sampleBuffer);
	});
}

@end
