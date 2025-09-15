import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';
import type { EventEmitter } from 'react-native/Libraries/Types/CodegenTypes';

export type LocalCandidate = {
  pc: string;
  candidate: string;
  mid: string;
};

export type NativeTransceiver = {
  kind: string;
  direction: string;
  sendms: string;
  recvms: string;
};

export interface Spec extends TurboModule {
  createPeerConnection(servers: string[]): string;
  closePeerConnection(pc: string): void;

  createMediaStreamTrack(kind: string): string;
  stopMediaStreamTrack(id: string): void;

  createRTCRtpTransceiver(
    pc: string,
    index: number,
    kind: string,
    direction: string,
    sendms: string,
    recvms: string
  ): string;
  stopRTCTransceiver(id: string): void;

  createOffer(pc: string): string;
  createAnswer(pc: string): string;

  getLocalDescription(pc: string): string;
  setLocalDescription(pc: string, sdp: string): void;

  getRemoteDescription(pc: string): string;
  setRemoteDescription(pc: string, sdp: string): void;

  addRemoteCandidate(pc: string, candidate: string, mid: string): void;

  onLocalCandidate: EventEmitter<LocalCandidate>;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeDatachannel');
