#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@protocol CameraDelegate <NSObject>

- (void)didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer;
@end

@interface CameraSession : NSObject
+ (instancetype)sharedInstance;

- (void)push:(NSString *)container;
- (void)pop:(NSString *)container;

@end
