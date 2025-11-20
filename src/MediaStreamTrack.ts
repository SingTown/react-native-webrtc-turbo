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
  id: string;
  readonly kind: 'audio' | 'video';
  readonly _device: MediaStreamTrackDevice;
  readonly _srcPipeId: string;
  readonly _dstPipeId: string;
  _subscriptionId: number = -1;

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
    this._srcPipeId = uuidv4();
    this._dstPipeId = uuidv4();
    this._enable();

    if (device === 'camera') {
      NativeMediaDevice.cameraAddPipe(this._srcPipeId);
    } else if (device === 'microphone') {
      NativeMediaDevice.microphoneAddPipe(this._srcPipeId);
    }
  }

  _enable() {
    if (this._subscriptionId > 0) {
      return;
    }
    this._subscriptionId = NativeDatachannel.forwardPipe(
      this._srcPipeId,
      this._dstPipeId
    );
  }

  _disable() {
    if (this._subscriptionId < 0) {
      return;
    }
    NativeDatachannel.unsubscribe(this._subscriptionId);
    this._subscriptionId = -1;
  }

  get enabled(): boolean {
    return this._subscriptionId > 0;
  }

  set enabled(value: boolean) {
    if (value) {
      this._enable();
    } else {
      this._disable();
    }
  }

  stop() {
    if (this._device === 'camera') {
      NativeMediaDevice.cameraRemovePipe(this._srcPipeId);
    } else if (this._device === 'microphone') {
      NativeMediaDevice.microphoneRemovePipe(this._srcPipeId);
    }
  }
}
