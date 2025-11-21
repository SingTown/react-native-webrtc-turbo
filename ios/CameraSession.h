#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@protocol CameraDelegate <NSObject>

- (void)didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer;
@end

@interface CameraSession : NSObject
+ (instancetype)sharedInstance;

- (void)addPipe:(NSString *)pipeId;
- (void)removePipe:(NSString *)pipeId;

@end
