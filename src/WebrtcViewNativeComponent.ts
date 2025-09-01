import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent';
import type { ViewProps } from 'react-native';

interface NativeProps extends ViewProps {
  videoStreamTrackId: string;
  audioStreamTrackId: string;
}

export default codegenNativeComponent<NativeProps>('WebrtcFabric');
