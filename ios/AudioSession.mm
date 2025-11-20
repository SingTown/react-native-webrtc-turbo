#import "AudioSession.h"
#import "log.h"
#import "framepipe.h"

static AudioSession *_sharedInstance = nil;

@interface AudioSession ()
@property (nonatomic, strong) dispatch_queue_t audioInQueue;
@property (nonatomic, strong) dispatch_queue_t audioOutQueue;
@property (nonatomic, strong) NSMutableArray<NSString *> *microphonePipes;
@property (nonatomic, assign) int subscriptionId;
@property (nonatomic, assign) int64_t ptsBase;
@property (nonatomic, strong) AVAudioSession *audioSession;
@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) AVAudioPlayerNode *playerNode;
@property (nonatomic, strong) AVAudioMixerNode *mixerNode;
@end

@implementation AudioSession

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _sharedInstance = [[AudioSession alloc] init];
  });
  return _sharedInstance;
}

- (instancetype)init {
  self = [super init];
  self.ptsBase = -1;
  self.subscriptionId = -1;
  self.microphonePipes = [NSMutableArray array];
  self.audioInQueue = dispatch_queue_create("audio.session.queue.in", DISPATCH_QUEUE_SERIAL);
  self.audioOutQueue = dispatch_queue_create("audio.session.queue.out", DISPATCH_QUEUE_SERIAL);
  
  self.audioSession = [AVAudioSession sharedInstance];
  [self.audioSession setCategory:AVAudioSessionCategoryPlayAndRecord
                            mode:AVAudioSessionModeDefault
                         options:AVAudioSessionCategoryOptionAllowBluetooth |
   AVAudioSessionCategoryOptionDefaultToSpeaker
                           error:nil];
  [self.audioSession setPreferredIOBufferDuration:0.02 error:nil];
  [self.audioSession setActive:YES error:nil];
  
  [self setupAudio];
  
  
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(handleInterruption:)
                                               name:AVAudioSessionInterruptionNotification
                                             object:nil];
  
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(handleRouteChange:)
                                               name:AVAudioSessionRouteChangeNotification
                                             object:nil];
  
  return self;
}

- (void)setupAudio{
  self.audioEngine = [[AVAudioEngine alloc] init];
  
  self.playerNode = [[AVAudioPlayerNode alloc] init];
  self.mixerNode = self.audioEngine.mainMixerNode;
  [self.audioEngine attachNode:self.playerNode];
  [self.audioEngine connect:self.playerNode to:self.mixerNode format:nil];
  
  AVAudioInputNode *inputNode = self.audioEngine.inputNode;
  [inputNode removeTapOnBus:0];
  [inputNode installTapOnBus:0 bufferSize:960 format:nil block:^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {
    [self handleInputBuffer:buffer atTime:when];
  }];
  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:nil];
}

- (void)handleInputBuffer:(AVAudioPCMBuffer *)buffer atTime:(AVAudioTime *)when {
  if (self.microphonePipes.count == 0) return;
  if (self.ptsBase == -1) {
    self.ptsBase = when.sampleTime;
  }
  
  std::shared_ptr<AVFrame> frame;
  int pts = when.sampleTime - self.ptsBase;
  if (buffer.format.isInterleaved) {
    frame = createAudioFrame(AV_SAMPLE_FMT_FLT, pts, buffer.format.sampleRate,
                             buffer.format.channelCount, buffer.frameLength);
    memcpy(frame->data[0], buffer.floatChannelData[0], buffer.frameLength * sizeof(float) * buffer.format.channelCount);
  } else {
    frame = createAudioFrame(AV_SAMPLE_FMT_FLTP, pts, buffer.format.sampleRate,
                             buffer.format.channelCount, buffer.frameLength);
    memcpy(frame->data[0], buffer.floatChannelData[0], buffer.frameLength * sizeof(float));
    if (buffer.format.channelCount == 2) {
      memcpy(frame->data[1], buffer.floatChannelData[1], buffer.frameLength * sizeof(float));
    }
  }
  
  for (NSString *pipeId in self.microphonePipes) {
    std::string cppStr = [pipeId UTF8String];
    if (!cppStr.empty()) {
      publish(cppStr, frame);
    }
  }
}

- (void)microphoneAddPipe:(NSString *)pipeId {
  if ([self.microphonePipes containsObject:pipeId]) {
    return;
  }
  [self.microphonePipes addObject:pipeId];
  [self.audioSession setActive:YES error:nil];
}

- (void)microphoneRemovePipe:(NSString *)pipeId {
  if (![self.microphonePipes containsObject:pipeId]) {
    return;
  }
  [self.microphonePipes removeObject:pipeId];
  if (self.microphonePipes.count == 0) {
    if (self.subscriptionId < 0) {
      [self.audioSession setActive:NO error:nil];
    }
  }
}

- (void)soundAddPipe:(NSString *)pipeId {
  if ([self subscriptionId] > 0) {
    unsubscribe([self subscriptionId]);
  }
  auto resampler = std::make_shared<Resampler>();
  std::string cppStr = [pipeId UTF8String];
  subscribe(cppStr, [self, resampler](std::shared_ptr<AVFrame> frame) {
    [self playAudio:frame resampler:resampler];
  });
  [self.audioSession setActive:YES error:nil];
  [self.playerNode play];
  [self.audioEngine startAndReturnError:nil];
}

- (void)soundRemovePipe:(NSString *)pipeId {
  [self.playerNode stop];
  [self.audioEngine stop];
  if (self.microphonePipes.count == 0) {
    [self.audioSession setActive:NO error:nil];
  }
}

#pragma mark - Notification Handlers

- (void)handleInterruption:(NSNotification *)notification {
  NSNumber *interruptionType = notification.userInfo[AVAudioSessionInterruptionTypeKey];
  
  switch (interruptionType.unsignedIntegerValue) {
    case AVAudioSessionInterruptionTypeBegan:
      LOGI("Audio session interruption began - stopping capture");
      [self.playerNode stop];
      [self.audioEngine stop];
      break;
      
    case AVAudioSessionInterruptionTypeEnded: {
      LOGI("Audio session interruption ended");
      NSNumber *interruptionOptions = notification.userInfo[AVAudioSessionInterruptionOptionKey];
      BOOL shouldResume = (interruptionOptions.unsignedIntegerValue & AVAudioSessionInterruptionOptionShouldResume) != 0;
      
      if (shouldResume) {
        [self.playerNode stop];
        [self.audioEngine stop];
        [self.audioEngine reset];
        [self setupAudio];
      }
      break;
    }
      
    default:
      break;
  }
}

- (void)handleRouteChange:(NSNotification *)notification {
  NSDictionary *userInfo = notification.userInfo;
  AVAudioSessionRouteChangeReason reason = (AVAudioSessionRouteChangeReason)[userInfo[AVAudioSessionRouteChangeReasonKey] unsignedIntegerValue];
  if (reason == AVAudioSessionRouteChangeReasonNewDeviceAvailable ||
      reason == AVAudioSessionRouteChangeReasonOverride
      ) {
    LOGI("Audio session route change detected");
    [self.playerNode stop];
    [self.audioEngine stop];
    [self.audioEngine reset];
    [self setupAudio];
  }
}

- (void)playAudio:(std::shared_ptr<AVFrame>)raw resampler:(std::shared_ptr<Resampler>)resampler {
  AVAudioFormat *format = [self.playerNode outputFormatForBus:0];
  AVSampleFormat avSampleFormat = AV_SAMPLE_FMT_NONE;
  if (format.isInterleaved) {
    avSampleFormat = AV_SAMPLE_FMT_FLT;
  } else {
    avSampleFormat = AV_SAMPLE_FMT_FLTP;
  }
  auto frame = resampler->resample(raw, avSampleFormat,
                                   format.sampleRate,
                                   format.channelCount);
  
  [self.audioEngine startAndReturnError:nil];
  [self.playerNode play];
  
  AVAudioFrameCount frameCount = (AVAudioFrameCount)frame->nb_samples;
  AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:frameCount];
  
  if (format.isInterleaved) {
    memcpy(buffer.audioBufferList->mBuffers[0].mData, frame->data[0], frame->nb_samples * sizeof(float) * frame->ch_layout.nb_channels);
  } else {
    memcpy(buffer.audioBufferList->mBuffers[0].mData, frame->data[0], frame->nb_samples * sizeof(float));
    memcpy(buffer.audioBufferList->mBuffers[1].mData, frame->data[1], frame->nb_samples * sizeof(float));
  }
  buffer.frameLength = frameCount;
  
  dispatch_async(self.audioOutQueue, ^{
    [self.playerNode scheduleBuffer:buffer completionHandler:nil];
  });
}


@end
