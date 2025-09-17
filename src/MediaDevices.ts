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
    const videoTrack = new MediaStreamTrack('video');
    await NativeMediaDevice.createCamera(videoTrack._sourceId);
    mediaStream.addTrack(videoTrack);
  }
  if (constraints.audio) {
    const allow = await NativeMediaDevice.requestPermission('microphone');
    if (!allow) {
      throw new Error('Microphone permission denied');
    }
    const audioTrack = new MediaStreamTrack('audio');
    await NativeMediaDevice.createAudio(audioTrack._sourceId);
    mediaStream.addTrack(audioTrack);
  }
  return mediaStream;
}

export const MediaDevices = {
  getUserMedia,
};
