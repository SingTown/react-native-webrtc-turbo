#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "MediaStreamTrack.h"

NS_ASSUME_NONNULL_BEGIN

@interface AudioSession : NSObject

+ (instancetype)sharedInstance;

- (void)capturePushMediaStreamTrack:(NSString *)id;
- (void)capturePopMediaStreamTrack:(NSString *)id;
- (void)playerPushMediaStreamTrack:(NSString *)id;
- (void)playerPopMediaStreamTrack:(NSString *)id;

@end

NS_ASSUME_NONNULL_END
