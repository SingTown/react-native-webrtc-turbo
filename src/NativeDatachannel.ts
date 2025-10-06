import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';
import type { EventEmitter } from 'react-native/Libraries/Types/CodegenTypes';

export type TrackEvent = {
  pc: string;
  mid: string;
  trackId: string;
  streamIds: string[];
};

export type ConnectionStateChangeEvent = {
  pc: string;
  state: string;
};

export type GatheringStateChangeEvent = {
  pc: string;
  state: string;
};

export type LocalCandidateEvent = {
  pc: string;
  candidate: string | null;
  mid: string | null;
};

export interface Spec extends TurboModule {
  createPeerConnection(servers: string[]): string;
  closePeerConnection(pc: string): void;

  getGatheringState(pc: string): string;
  getPeerConnectionState(pc: string): string;

  createMediaContainer(kind: string): string;
  removeMediaContainer(id: string): void;

  createRTCRtpTransceiver(
    pc: string,
    index: number,
    kind: string,
    direction: string,
    sendms: string,
    recvms: string,
    msids: string[],
    trackid: string | null
  ): string;
  stopRTCTransceiver(id: string): void;

  createOffer(pc: string): string;
  createAnswer(pc: string): string;

  getLocalDescription(pc: string): string;
  setLocalDescription(pc: string, sdp: string): void;

  getRemoteDescription(pc: string): string;
  setRemoteDescription(pc: string, sdp: string): void;

  addRemoteCandidate(pc: string, candidate: string, mid: string): void;

  onTrack: EventEmitter<TrackEvent>;
  onConnectionStateChange: EventEmitter<ConnectionStateChangeEvent>;
  onGatheringStateChange: EventEmitter<GatheringStateChangeEvent>;
  onLocalCandidate: EventEmitter<LocalCandidateEvent>;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeDatachannel');
