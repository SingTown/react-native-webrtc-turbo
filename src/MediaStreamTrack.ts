import NativeDatachannelModule from './NativeDatachannelModule';

export class MediaStreamTrack {
  enabled: boolean = true;
  readonly kind: 'audio' | 'video';
  readonly id: string;

  constructor(kind: 'audio' | 'video') {
    this.kind = kind;
    this.id = NativeDatachannelModule.createMediaStreamTrack();
  }
}
