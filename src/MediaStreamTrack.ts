import NativeDatachannel from './NativeDatachannel';
import NativeMediaDevice from './NativeMediaDevice';
import { uuidv4 } from './uuid';

export class MediaStreamTrack {
  enabled: boolean = true;
  id: string;
  readonly kind: 'audio' | 'video';
  readonly _sourceId: string;

  constructor(kind: 'audio' | 'video') {
    this.kind = kind;
    this.id = uuidv4();
    this._sourceId = NativeDatachannel.createMediaStreamTrack(kind);
  }

  stop() {
    NativeDatachannel.stopMediaStreamTrack(this._sourceId);
    if (this.kind === 'video') {
      NativeMediaDevice.deleteCamera(this._sourceId);
    } else if (this.kind === 'audio') {
      NativeMediaDevice.deleteAudio(this._sourceId);
    }
  }
}
