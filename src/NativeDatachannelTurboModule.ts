import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';
import type { EventEmitter } from 'react-native/Libraries/Types/CodegenTypes';

export type LocalCandidate = {
  pc: number;
  candidate: string;
  mid: string;
};

export type Message = {
  id: number;
  base64: string;
};

export interface Spec extends TurboModule {
  createPeerConnection(servers: string[]): number;
  closePeerConnection(pc: number): void;
  deletePeerConnection(pc: number): void;

  createOffer(pc: number): string;
  createAnswer(pc: number): string;

  setLocalDescription(pc: number, type: string): void;
  getLocalDescription(pc: number): string;

  setRemoteDescription(pc: number, sdp: string, type: string): void;
  getRemoteDescription(pc: number): string;

  addTrack(pc: number, sdp: string): number;
  deleteTrack(tr: number): void;

  addRemoteCandidate(pc: number, candidate: string, mid: string): void;

  onLocalCandidate: EventEmitter<LocalCandidate>;
  onMessage: EventEmitter<Message>;
}

export default TurboModuleRegistry.getEnforcing<Spec>('DatachannelTurboModule');
