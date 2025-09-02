#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@protocol CameraDelegate <NSObject>

- (void)didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer;
@end

@interface Camera : NSObject

- (instancetype)init:(NSString *)ms;

@property (nonatomic, weak) id<CameraDelegate> delegate;
@property (nonatomic, assign) int64_t ptsBase;
@property (nonatomic, readonly) BOOL isRunning;

- (BOOL)startCapture:(NSError **)error;
- (void)stopCapture;

@end
