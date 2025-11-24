import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent';
import type { ViewProps } from 'react-native';

interface NativeProps extends ViewProps {
  videoPipeId: string;
  audioPipeId: string;
  resizeMode: string;
}

export default codegenNativeComponent<NativeProps>('WebrtcFabric');
