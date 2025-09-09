import { MediaStreamTrack } from './MediaStreamTrack';
import NativeDatachannel from './NativeDatachannel';
export type RTCRtpTransceiverDirection =
  | 'inactive'
  | 'recvonly'
  | 'sendonly'
  | 'sendrecv'
  | 'stopped';

export interface RTCRtpTransceiverInit {
  direction?: RTCRtpTransceiverDirection;
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
  currentDirection: RTCRtpTransceiverDirection | null = null;
  direction: RTCRtpTransceiverDirection;
  kind: 'audio' | 'video';
  readonly receiver: RTCRtpReceiver;
  readonly sender: RTCRtpSender;

  constructor(
    trackOrKind: MediaStreamTrack | 'audio' | 'video',
    init?: RTCRtpTransceiverInit
  ) {
    this.direction = init?.direction || 'sendrecv';
    let sendTrack: MediaStreamTrack | null = null;
    let recvTrack: MediaStreamTrack | null = null;
    if (trackOrKind instanceof MediaStreamTrack) {
      this.kind = trackOrKind.kind;
      if (this.direction === 'sendonly') {
        sendTrack = trackOrKind;
      } else if (this.direction === 'recvonly') {
        recvTrack = trackOrKind;
      }
    } else {
      this.kind = trackOrKind;
      if (this.direction.includes('send')) {
        sendTrack = new MediaStreamTrack(this.kind);
      }
      if (this.direction.includes('recv')) {
        recvTrack = new MediaStreamTrack(this.kind);
      }
    }

    this.sender = new RTCRtpSender(sendTrack);
    this.receiver = new RTCRtpReceiver(recvTrack);
  }

  stop() {
    this.id && NativeDatachannel.stopRTCTransceiver(this.id);
  }
}
