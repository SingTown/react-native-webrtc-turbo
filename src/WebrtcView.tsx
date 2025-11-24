import WebrtcFabric from './WebrtcViewNativeComponent';
import { MediaStream } from './MediaStream';
import type { ViewStyle } from 'react-native';

type WebrtcViewProps = {
  stream: MediaStream | null;
  resizeMode?: 'contain' | 'cover' | 'fill';
  style?: ViewStyle;
};

export function WebrtcView(props: WebrtcViewProps) {
  const videoTrack = props.stream?.getVideoTracks()[0];
  const audioTrack = props.stream?.getAudioTracks()[0];
  return (
    <WebrtcFabric
      style={props.style}
      videoPipeId={videoTrack ? videoTrack._dstPipeId : ''}
      audioPipeId={audioTrack ? audioTrack._dstPipeId : ''}
      resizeMode={props.resizeMode || 'contain'}
    />
  );
}
