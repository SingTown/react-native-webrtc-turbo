import { TurboModuleRegistry, type TurboModule } from 'react-native';

export interface Spec extends TurboModule {
  requestPermission(name: 'camera' | 'microphone'): Promise<boolean>;

  cameraPush(container: string): void;
  cameraPop(container: string): void;

  microphonePush(container: string): void;
  microphonePop(container: string): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeMediaDevice');
