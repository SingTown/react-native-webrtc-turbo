import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
  reverseString(input: string): string;
}

export default TurboModuleRegistry.getEnforcing<Spec>(
  'NativeDatachannelModule'
);
