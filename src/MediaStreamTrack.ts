import NativeDatachannel from './NativeDatachannel';

export class MediaStreamTrack {
  enabled: boolean = true;
  readonly kind: 'audio' | 'video';
  readonly id: string;

  constructor(kind: 'audio' | 'video') {
    this.kind = kind;
    this.id = NativeDatachannel.createMediaStreamTrack();
  }
}
