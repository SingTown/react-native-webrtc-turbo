import { MediaStream } from './MediaStream';
import { MediaStreamTrack } from './MediaStreamTrack';
import NativeMediaDevice from './NativeMediaDevice';
import { uuidv4 } from './uuid';

export interface MediaStreamConstraints {
  audio?: boolean;
  video?: boolean;
}

async function getUserMedia(
  constraints: MediaStreamConstraints
): Promise<MediaStream> {
  const msid = uuidv4();
  const mediaStream = new MediaStream(msid);
  if (constraints.video) {
    await NativeMediaDevice.requestPermission('camera');
    const videoTrack = new MediaStreamTrack('camera');
    mediaStream.addTrack(videoTrack);
  }
  if (constraints.audio) {
    await NativeMediaDevice.requestPermission('microphone');
    const audioTrack = new MediaStreamTrack('microphone');
    mediaStream.addTrack(audioTrack);
  }
  return mediaStream;
}

export const MediaDevices = {
  getUserMedia,
};
