import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent';
import type { ViewProps } from 'react-native';

interface NativeProps extends ViewProps {
  videoContainer: string;
  audioContainer: string;
}

export default codegenNativeComponent<NativeProps>('WebrtcFabric');
