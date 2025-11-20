import { TurboModuleRegistry, type TurboModule } from 'react-native';

export interface Spec extends TurboModule {
  requestPermission(name: 'camera' | 'microphone'): Promise<boolean>;

  cameraAddPipe(pipeId: string): void;
  cameraRemovePipe(pipeId: string): void;

  microphoneAddPipe(pipeId: string): void;
  microphoneRemovePipe(pipeId: string): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeMediaDevice');
