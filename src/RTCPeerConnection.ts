import type { EventSubscription } from 'react-native';
import NativeDatachannel from './NativeDatachannel';
import { RTCRtpTransceiver } from './RTCRtpTransceiver';
import type { RTCRtpTransceiverInit } from './RTCRtpTransceiver';
import { MediaStreamTrack } from './MediaStreamTrack';
import { MediaStream } from './MediaStream';

export interface RTCIceServer {
  credential?: string;
  urls: string[] | string;
  username?: string;
}

export interface RTCConfiguration {
  iceServers?: RTCIceServer[];
}

export type RTCSdpType = 'answer' | 'offer';
export interface RTCSessionDescriptionInit {
  sdp?: string;
  type: RTCSdpType;
}

export type RTCTrackEvent = {
  track: MediaStreamTrack;
  streams: MediaStream[];
};

export interface RTCIceCandidateInit {
  candidate?: string;
  sdpMid?: string | null;
}

export class RTCIceCandidate {
  candidate: string;
  sdpMid: string | null;

  constructor(init: RTCIceCandidateInit) {
    this.candidate = init.candidate || '';
    this.sdpMid = init.sdpMid || null;
  }
}

export type RTCPeerConnectionIceEvent = {
  candidate: RTCIceCandidate | null;
};

export class RTCPeerConnection {
  public pc: string;
  private transceivers: RTCRtpTransceiver[] = [];
  private onConnectionStateChangeCallback: EventSubscription;
  private onIceGatheringStateChangeCallback: EventSubscription;
  private onLocalCandidateCallback: EventSubscription;
  private onTrackCallback: EventSubscription;

  private streams: Map<string, MediaStream> = new Map();

  public onconnectionstatechange: (() => void) | null = null;
  public onicegatheringstatechange: (() => void) | null = null;
  public onicecandidate:
    | ((candidate: RTCPeerConnectionIceEvent) => void)
    | null = null;
  public ontrack: ((event: RTCTrackEvent) => void) | null = null;

  constructor(configuration?: RTCConfiguration) {
    const servers = this.convertIceServersToUrls(
      configuration?.iceServers || []
    );
    this.pc = NativeDatachannel.createPeerConnection(servers);

    this.onConnectionStateChangeCallback =
      NativeDatachannel.onConnectionStateChange((obj) => {
        if (obj.pc !== this.pc || !this.onconnectionstatechange) {
          return;
        }
        this.onconnectionstatechange();
      });

    this.onIceGatheringStateChangeCallback =
      NativeDatachannel.onGatheringStateChange((obj) => {
        if (obj.pc !== this.pc || !this.onicegatheringstatechange) {
          return;
        }
        this.onicegatheringstatechange();
      });

    this.onLocalCandidateCallback = NativeDatachannel.onLocalCandidate(
      (obj) => {
        if (obj.pc !== this.pc || !this.onicecandidate) {
          return;
        }
        if (!obj.candidate) {
          this.onicecandidate({ candidate: null });
          return;
        } else {
          this.onicecandidate({
            candidate: new RTCIceCandidate({
              candidate: obj.candidate,
              sdpMid: obj.mid,
            }),
          });
        }
      }
    );
    this.onTrackCallback = NativeDatachannel.onTrack((obj) => {
      if (obj.pc !== this.pc || !this.ontrack) {
        return;
      }
      const transceiver = this.transceivers.find((t) => t.mid === obj.mid);
      if (!transceiver) {
        return;
      }
      const track = transceiver.receiver.track;
      if (!track) {
        return;
      }
      track.id = obj.trackId;
      for (const msid of obj.streamIds) {
        let stream = this.streams.get(msid);
        if (!stream) {
          stream = new MediaStream(msid);
          this.streams.set(msid, stream);
        }
        stream.addTrack(track);
        transceiver.streams.push(stream);
      }

      this.ontrack({
        track: track,
        streams: transceiver.streams,
      });
    });
  }

  private convertIceServersToUrls(iceServers: RTCIceServer[]): string[] {
    let servers: string[] = [];
    for (const iceServer of iceServers) {
      let urls: string[] = [];
      if (Array.isArray(iceServer.urls)) {
        urls = iceServer.urls;
      } else {
        urls = [iceServer.urls];
      }
      const password = iceServer.credential || '';
      const username = iceServer.username || '';

      for (const url of urls) {
        const protocol = url.split(':')[0];
        const host = url.split(':')[1];
        const port = url.split(':')[2];
        if (!protocol || !host || !port) {
          throw new Error(`Invalid ICE server URL: ${url}`);
        }
        if (password && username) {
          servers.push(`${protocol}:${username}:${password}@${host}:${port}`);
        } else {
          servers.push(`${protocol}:${host}:${port}`);
        }
      }
    }
    return servers;
  }

  close() {
    this.onConnectionStateChangeCallback.remove();
    this.onIceGatheringStateChangeCallback.remove();
    this.onLocalCandidateCallback.remove();
    this.onTrackCallback.remove();
    this.streams.clear();
    NativeDatachannel.closePeerConnection(this.pc);
  }

  get connectionState(): string {
    return NativeDatachannel.getPeerConnectionState(this.pc);
  }

  get iceGatheringState(): string {
    return NativeDatachannel.getGatheringState(this.pc);
  }

  addTransceiver(
    trackOrKind: MediaStreamTrack | 'audio' | 'video',
    init?: RTCRtpTransceiverInit
  ) {
    const mid = `${this.transceivers.length}`;
    const transceiver = new RTCRtpTransceiver(trackOrKind, mid, init);
    this.transceivers.push(transceiver);
    return transceiver;
  }

  getTransceivers() {
    return this.transceivers;
  }

  private createRTCRtpTransceivers() {
    for (let i = 0; i < this.transceivers.length; i++) {
      const t = this.transceivers[i];
      if (!t) continue;
      const sendms = t?.sender.track?._sourceId || '';
      const recvms = t?.receiver.track?._sourceId || '';
      if (!t.id) {
        const msids = t.streams.map((s) => s.msid);
        const id = NativeDatachannel.createRTCRtpTransceiver(
          this.pc,
          i,
          t.kind,
          t.direction,
          sendms,
          recvms,
          msids,
          t.sender.track?.id || null
        );
        t.id = id;
      }
    }
  }

  createOffer(): Promise<RTCSessionDescriptionInit> {
    this.createRTCRtpTransceivers();
    const sdp = NativeDatachannel.createOffer(this.pc);
    return Promise.resolve({ sdp, type: 'offer' });
  }

  createAnswer(): Promise<RTCSessionDescriptionInit> {
    this.createRTCRtpTransceivers();
    const sdp = NativeDatachannel.createAnswer(this.pc);
    return Promise.resolve({ sdp, type: 'answer' });
  }

  get localDescription(): string {
    return NativeDatachannel.getLocalDescription(this.pc);
  }

  setLocalDescription(description: RTCSessionDescriptionInit): Promise<void> {
    NativeDatachannel.setLocalDescription(this.pc, description.sdp || '');
    return Promise.resolve();
  }

  get remoteDescription(): string {
    return NativeDatachannel.getRemoteDescription(this.pc);
  }

  setRemoteDescription(description: RTCSessionDescriptionInit): Promise<void> {
    NativeDatachannel.setRemoteDescription(this.pc, description.sdp || '');
    return Promise.resolve();
  }

  addIceCandidate(candidate?: RTCIceCandidateInit | null): Promise<void> {
    let mid = '0';
    if (candidate?.sdpMid) {
      mid = candidate.sdpMid;
    }
    let str = '';
    if (candidate?.candidate) {
      str = candidate.candidate;
    }
    if (str) {
      NativeDatachannel.addRemoteCandidate(this.pc, str, mid);
    }
    return Promise.resolve();
  }
}
