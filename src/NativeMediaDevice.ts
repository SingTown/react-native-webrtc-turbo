import { TurboModuleRegistry, type TurboModule } from 'react-native';

export interface Spec extends TurboModule {
  requestPermission(name: 'camera' | 'microphone'): Promise<boolean>;

  createCamera(ms: string): Promise<string>;
  deleteCamera(ms: string): void;

  createAudio(ms: string): Promise<string>;
  deleteAudio(ms: string): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeMediaDevice');
