import { MediaStreamTrack } from './MediaStreamTrack';
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
  readonly receiver: RTCRtpReceiver;
  readonly sender: RTCRtpSender;

  constructor(
    pcid: string,
    kind: 'audio' | 'video',
    init?: RTCRtpTransceiverInit
  ) {
    const direction = init?.direction || 'sendrecv';
    let sendTrack: MediaStreamTrack | null = null;
    let recvTrack: MediaStreamTrack | null = null;

    if (direction.includes('send')) {
      sendTrack = new MediaStreamTrack(pcid, kind, 'sendonly');
    }
    if (direction.includes('recv')) {
      recvTrack = new MediaStreamTrack(pcid, kind, 'recvonly');
    }

    this.sender = new RTCRtpSender(sendTrack);
    this.receiver = new RTCRtpReceiver(recvTrack);
  }
}
