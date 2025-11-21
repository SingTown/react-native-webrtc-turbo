import NativeDatachannel from './NativeDatachannel';
import { MediaStream } from './MediaStream';

export class MediaRecorder {
  private audioPipeId: string;
  private videoPipeId: string;
  private subscriptionId: number;

  constructor(stream: MediaStream) {
    const audioTrack = stream.getAudioTracks()[0];
    const videoTrack = stream.getVideoTracks()[0];

    this.audioPipeId = audioTrack ? audioTrack._dstPipeId : '';
    this.videoPipeId = videoTrack ? videoTrack._dstPipeId : '';
    this.subscriptionId = -1;
  }
  async takePhoto(file: string) {
    await NativeDatachannel.takePhoto(file, this.videoPipeId);
  }
  startRecording(file: string) {
    this.subscriptionId = NativeDatachannel.startRecording(
      file,
      this.audioPipeId,
      this.videoPipeId
    );
  }
  stopRecording() {
    if (this.subscriptionId) {
      NativeDatachannel.unsubscribe(this.subscriptionId);
      this.subscriptionId = -1;
    }
  }
}
