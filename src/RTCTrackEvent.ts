import { RTCRtpTransceiver, RTCRtpReceiver } from './RTCRtpTransceiver';
import { MediaStream } from './MediaStream';

export class RTCTrackEvent {
  readonly receiver: RTCRtpReceiver;
  readonly transceiver: RTCRtpTransceiver;
  readonly streams: MediaStream[];

  constructor(transceiver: RTCRtpTransceiver) {
    this.transceiver = transceiver;
    this.receiver = transceiver.receiver;
    this.streams = [];
    const stream = new MediaStream();
    this.receiver.track && stream.addTrack(this.receiver.track);
    this.streams.push(stream);
  }
}
