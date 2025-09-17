import WebrtcFabric from './WebrtcViewNativeComponent';
import { MediaStream } from './MediaStream';
import type { ViewStyle } from 'react-native';

type WebrtcViewProps = {
  stream: MediaStream | null;
  style?: ViewStyle;
};

export function WebrtcView(props: WebrtcViewProps) {
  const videoTrack = props.stream?.getVideoTracks()[0];
  const audioTrack = props.stream?.getAudioTracks()[0];
  let videoStreamTrackId = videoTrack ? videoTrack._sourceId : '';
  let audioStreamTrackId = audioTrack ? audioTrack._sourceId : '';
  return (
    <WebrtcFabric
      style={props.style}
      videoStreamTrackId={videoStreamTrackId}
      audioStreamTrackId={audioStreamTrackId}
    />
  );
}
