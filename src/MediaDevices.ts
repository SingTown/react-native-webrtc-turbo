import { MediaStream } from './MediaStream';
import { MediaStreamTrack } from './MediaStreamTrack';
import NativeMediaDevice from './NativeMediaDevice';

export interface MediaStreamConstraints {
  audio?: boolean;
  video?: boolean;
}

async function getUserMedia(
  constraints: MediaStreamConstraints
): Promise<MediaStream> {
  const mediaStream = new MediaStream();
  if (constraints.video) {
    const allow = await NativeMediaDevice.requestCameraPermission();
    if (!allow) {
      throw new Error('Camera permission denied');
    }
    const videoTrack = new MediaStreamTrack('video');
    await NativeMediaDevice.createCamera(videoTrack.id);
    mediaStream.addTrack(videoTrack);
  }
  if (constraints.audio) {
    const audioTrack = new MediaStreamTrack('audio');
    mediaStream.addTrack(audioTrack);
  }
  return mediaStream;
}

export const MediaDevices = {
  getUserMedia,
};
