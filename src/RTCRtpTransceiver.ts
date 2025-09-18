import { MediaStreamTrack } from './MediaStreamTrack';
import { MediaStream } from './MediaStream';
import NativeDatachannel from './NativeDatachannel';
export type RTCRtpTransceiverDirection =
  | 'inactive'
  | 'recvonly'
  | 'sendonly'
  | 'sendrecv'
  | 'stopped';

export interface RTCRtpTransceiverInit {
  direction?: RTCRtpTransceiverDirection;
  streams?: MediaStream[];
}

export class RTCRtpReceiver {
  readonly track: MediaStreamTrack | null;

  constructor(track: MediaStreamTrack | null) {
    this.track = track;
  }
}

export class RTCRtpSender {
  readonly track: MediaStreamTrack | null;

  constructor(track: MediaStreamTrack | null) {
    this.track = track;
  }
}

export class RTCRtpTransceiver {
  id: string | null = null;
  mid: string;
  currentDirection: RTCRtpTransceiverDirection | null = null;
  direction: RTCRtpTransceiverDirection;
  kind: 'audio' | 'video';
  streams: MediaStream[];
  readonly receiver: RTCRtpReceiver;
  readonly sender: RTCRtpSender;

  constructor(
    trackOrKind: MediaStreamTrack | 'audio' | 'video',
    mid: string,
    init?: RTCRtpTransceiverInit
  ) {
    this.direction = init?.direction || 'sendrecv';
    this.streams = init?.streams || [];
    this.mid = mid;
    let sendTrack: MediaStreamTrack | null = null;
    if (trackOrKind instanceof MediaStreamTrack) {
      this.kind = trackOrKind.kind;
      if (this.direction.includes('send')) {
        sendTrack = trackOrKind;
      }
    } else {
      this.kind = trackOrKind;
      if (this.direction.includes('send')) {
        sendTrack = new MediaStreamTrack(this.kind);
      }
    }
    let recvTrack: MediaStreamTrack | null = null;
    if (this.direction.includes('recv')) {
      recvTrack = new MediaStreamTrack(this.kind);
    }
    this.sender = new RTCRtpSender(sendTrack);
    this.receiver = new RTCRtpReceiver(recvTrack);
  }

  stop() {
    this.id && NativeDatachannel.stopRTCTransceiver(this.id);
    this.streams = [];
  }
}
