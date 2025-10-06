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
    const allow = await NativeMediaDevice.requestPermission('camera');
    if (!allow) {
      throw new Error('Camera permission denied');
    }
    const videoTrack = new MediaStreamTrack('camera');
    mediaStream.addTrack(videoTrack);
  }
  if (constraints.audio) {
    const allow = await NativeMediaDevice.requestPermission('microphone');
    if (!allow) {
      throw new Error('Microphone permission denied');
    }
    const audioTrack = new MediaStreamTrack('microphone');
    mediaStream.addTrack(audioTrack);
  }
  return mediaStream;
}

export const MediaDevices = {
  getUserMedia,
};
