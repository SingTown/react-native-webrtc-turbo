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
  readonly _containerId: string;

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
    this._containerId = NativeDatachannel.createMediaContainer(this.kind);

    if (device === 'camera') {
      NativeMediaDevice.cameraAddContainer(this._containerId);
    } else if (device === 'microphone') {
      NativeMediaDevice.microphoneAddContainer(this._containerId);
    }
  }

  stop() {
    NativeDatachannel.removeMediaContainer(this._containerId);
    if (this._device === 'camera') {
      NativeMediaDevice.cameraRemoveContainer(this._containerId);
    } else if (this._device === 'microphone') {
      NativeMediaDevice.microphoneRemoveContainer(this._containerId);
    }
  }
}
