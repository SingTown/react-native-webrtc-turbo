import { TurboModuleRegistry, type TurboModule } from 'react-native';

export interface Spec extends TurboModule {
  requestCameraPermission(): Promise<boolean>;
  createCamera(ms: string): Promise<string>;
  deleteCamera(ms: string): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeMediaDevice');
