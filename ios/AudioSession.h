#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "MediaContainer.h"

NS_ASSUME_NONNULL_BEGIN

@interface AudioSession : NSObject

+ (instancetype)sharedInstance;

- (void)microphoneAddContainer:(NSString *)container;
- (void)microphoneRemoveContainer:(NSString *)container;
- (void)soundAddContainer:(NSString *)container;
- (void)soundRemoveContainer:(NSString *)container;

@end

NS_ASSUME_NONNULL_END
