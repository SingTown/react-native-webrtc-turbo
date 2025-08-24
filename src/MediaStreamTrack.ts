import NativeDatachannelModule from './NativeDatachannelModule';

export class MediaStreamTrack {
  enabled: boolean = true;
  readonly id: string;
  readonly kind: string;

  constructor(
    pcid: string,
    kind: 'audio' | 'video',
    direction: 'sendonly' | 'recvonly'
  ) {
    this.kind = kind;
    this.id = NativeDatachannelModule.addTrack(pcid, kind, direction);
  }
}
