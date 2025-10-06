import { TurboModuleRegistry, type TurboModule } from 'react-native';

export interface Spec extends TurboModule {
  requestPermission(name: 'camera' | 'microphone'): Promise<boolean>;

  cameraAddContainer(container: string): void;
  cameraRemoveContainer(container: string): void;

  microphoneAddContainer(container: string): void;
  microphoneRemoveContainer(container: string): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeMediaDevice');
