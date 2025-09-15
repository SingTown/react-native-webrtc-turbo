import NativeDatachannel from './NativeDatachannel';
import NativeMediaDevice from './NativeMediaDevice';

export class MediaStreamTrack {
  enabled: boolean = true;
  readonly kind: 'audio' | 'video';
  readonly id: string;

  constructor(kind: 'audio' | 'video') {
    this.kind = kind;
    this.id = NativeDatachannel.createMediaStreamTrack(kind);
  }

  stop() {
    NativeDatachannel.stopMediaStreamTrack(this.id);
    if (this.kind === 'video') {
      NativeMediaDevice.deleteCamera(this.id);
    } else if (this.kind === 'audio') {
      NativeMediaDevice.deleteAudio(this.id);
    }
  }
}
