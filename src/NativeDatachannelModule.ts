import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';
import type { EventEmitter } from 'react-native/Libraries/Types/CodegenTypes';

export type LocalCandidate = {
  pc: string;
  candidate: string;
  mid: string;
};

export interface Spec extends TurboModule {
  createPeerConnection(servers: string[]): string;
  closePeerConnection(pc: string): void;
  deletePeerConnection(pc: string): void;

  addTrack(pc: string, kind: string, direction: string): string;

  createOffer(pc: string): string;
  createAnswer(pc: string): string;

  getLocalDescription(pc: string): string;
  setLocalDescription(pc: string, type: string): void;

  getRemoteDescription(pc: string): string;
  setRemoteDescription(pc: string, sdp: string, type: string): void;

  addRemoteCandidate(pc: string, candidate: string, mid: string): void;

  onLocalCandidate: EventEmitter<LocalCandidate>;
}

export default TurboModuleRegistry.getEnforcing<Spec>(
  'NativeDatachannelModule'
);
