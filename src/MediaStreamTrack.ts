import NativeDatachannel from './NativeDatachannel';
import NativeMediaDevice from './NativeMediaDevice';
import { uuidv4 } from './uuid';

export type MediaStreamTrackDevice =
  | 'camera'
  | 'microphone'
  | 'screen'
  | 'sound'
  | 'video'
  | 'audio';

export class MediaStreamTrack {
  enabled: boolean = true;
  id: string;
  readonly kind: 'audio' | 'video';
  readonly _device: MediaStreamTrackDevice;
  readonly _sourceId: string;

  constructor(device: MediaStreamTrackDevice) {
    this.id = uuidv4();
    this._device = device;
    if (device === 'microphone' || device === 'sound' || device === 'audio') {
      this.kind = 'audio';
    } else if (
      device === 'camera' ||
      device === 'screen' ||
      device === 'video'
    ) {
      this.kind = 'video';
    } else {
      throw new Error('MediaStreamTrack device must be camera or microphone');
    }
    this._sourceId = NativeDatachannel.createMediaStreamTrack(this.kind);

    if (device === 'camera') {
      NativeMediaDevice.cameraPush(this._sourceId);
    } else if (device === 'microphone') {
      NativeMediaDevice.microphonePush(this._sourceId);
      // } else if (device === 'screen') {
      //   this.kind = 'video';
      //   // this._sourceId = NativeMediaDevice.createScreen();
      //   this._sourceId = NativeDatachannel.createMediaStreamTrack(this.kind);
      // } else if (device === 'sound') {
      //   this.kind = 'audio';
      //   // this._sourceId = NativeMediaDevice.createSound();
      //   this._sourceId = NativeDatachannel.createMediaStreamTrack(this.kind);
    }
  }

  stop() {
    NativeDatachannel.stopMediaStreamTrack(this._sourceId);
    if (this._device === 'camera') {
      NativeMediaDevice.cameraPop(this._sourceId);
    } else if (this._device === 'microphone') {
      NativeMediaDevice.microphonePop(this._sourceId);
    }
  }
}
