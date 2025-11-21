#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AudioSession : NSObject

+ (instancetype)sharedInstance;

- (void)microphoneAddPipe:(NSString *)pipeId;
- (void)microphoneRemovePipe:(NSString *)pipeId;
- (void)soundAddPipe:(NSString *)pipeId;
- (void)soundRemovePipe:(NSString *)pipeId;

@end

NS_ASSUME_NONNULL_END
