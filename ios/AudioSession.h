#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "MediaStreamTrack.h"

NS_ASSUME_NONNULL_BEGIN

@interface AudioSession : NSObject

+ (instancetype)sharedInstance;

- (void)microphonePush:(NSString *)container;
- (void)microphonePop:(NSString *)container;
- (void)soundPush:(NSString *)container;
- (void)soundPop:(NSString *)container;

@end

NS_ASSUME_NONNULL_END
