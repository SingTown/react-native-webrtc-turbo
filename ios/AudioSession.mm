#import "AudioSession.h"
#import "log.h"

static AudioSession *_sharedInstance = nil;

@interface AudioSession ()
@property (nonatomic, strong) dispatch_queue_t audioInQueue;
@property (nonatomic, strong) dispatch_queue_t audioOutQueue;
@property (nonatomic, strong) NSMutableArray<NSString *> *audioInStreamTrackIds;
@property (nonatomic, strong) NSMutableArray<NSString *> *audioOutStreamTrackIds;
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
  self.audioInStreamTrackIds = [NSMutableArray array];
  self.audioOutStreamTrackIds = [NSMutableArray array];
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
  
  [self scheduleNextAudioFrame];
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
  if (self.audioInStreamTrackIds.count == 0) return;
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
    memcpy(frame->data[1], buffer.floatChannelData[1], buffer.frameLength * sizeof(float));
  }
  
  for (NSString *streamId in self.audioInStreamTrackIds) {
    std::string cppStr = [streamId UTF8String];
    auto mediaStreamTrack = getMediaStreamTrack(cppStr);
    if (mediaStreamTrack) {
      mediaStreamTrack->push(frame);
    }
  }
}

- (void)scheduleNextAudioFrame {
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_MSEC)), dispatch_get_main_queue(), ^{
    [self playAudio];
    [self scheduleNextAudioFrame];
  });
}

- (void)capturePushMediaStreamTrack:(NSString *)id {
  [self.audioInStreamTrackIds addObject:id];
  [self.audioSession setActive:YES error:nil];
}

- (void)capturePopMediaStreamTrack:(NSString *)id {
  [self.audioInStreamTrackIds removeObject:id];
  if (self.audioInStreamTrackIds.count == 0) {
    if (self.audioOutStreamTrackIds.count == 0) {
      [self.audioSession setActive:NO error:nil];
    }
  }
}

- (void)playerPushMediaStreamTrack:(NSString *)id {
  [self.audioOutStreamTrackIds addObject:id];
  [self.audioSession setActive:YES error:nil];
  [self.playerNode play];
  [self.audioEngine startAndReturnError:nil];
}

- (void)playerPopMediaStreamTrack:(NSString *)id {
  [self.audioOutStreamTrackIds removeObject:id];
  if (self.audioOutStreamTrackIds.count == 0) {
    [self.playerNode stop];
    [self.audioEngine stop];
    if (self.audioInStreamTrackIds.count == 0) {
      [self.audioSession setActive:NO error:nil];
    }
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

- (void)playAudio {
  for (NSString *streamId in self.audioOutStreamTrackIds) {
    std::string cppStr = [streamId UTF8String];
    auto audioStreamTrack = getAudioStreamTrack(cppStr);
    if (!audioStreamTrack) {
      continue;
    }
    AVAudioFormat *format = [self.playerNode outputFormatForBus:0];
    AVSampleFormat avSampleFormat = AV_SAMPLE_FMT_NONE;
    if (format.isInterleaved) {
      avSampleFormat = AV_SAMPLE_FMT_FLT;
    } else {
      avSampleFormat = AV_SAMPLE_FMT_FLTP;
    }
    
    auto frame = audioStreamTrack->popAudio(avSampleFormat, format.sampleRate, format.channelCount);
    if (!frame) {
      continue;
    }
    
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
}


@end
