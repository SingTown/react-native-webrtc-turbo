import type { EventSubscription } from 'react-native';
import NativeDatachannel from './NativeDatachannel';
import { RTCRtpTransceiver } from './RTCRtpTransceiver';
import type { RTCRtpTransceiverInit } from './RTCRtpTransceiver';
import { RTCTrackEvent } from './RTCTrackEvent';
import type { MediaStreamTrack } from './MediaStreamTrack';

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

export class RTCPeerConnection {
  private pc: string;
  private transceivers: RTCRtpTransceiver[] = [];
  private onLocalCandidateCallback: EventSubscription;

  public onicecandidate: ((candidate: string) => void) | null = null;
  public ontrack: ((event: RTCTrackEvent) => void) | null = null;

  constructor(configuration?: RTCConfiguration) {
    const servers = this.convertIceServersToUrls(
      configuration?.iceServers || []
    );
    this.pc = NativeDatachannel.createPeerConnection(servers);
    this.onLocalCandidateCallback = NativeDatachannel.onLocalCandidate(
      (obj) => {
        if (obj.pc !== this.pc || !this.onicecandidate) {
          return;
        }
        this.onicecandidate(obj.candidate);
      }
    );
  }

  convertIceServersToUrls(iceServers: RTCIceServer[]): string[] {
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
    this.onLocalCandidateCallback.remove();
    NativeDatachannel.closePeerConnection(this.pc);
  }

  addTransceiver(
    trackOrKind: MediaStreamTrack | 'audio' | 'video',
    init?: RTCRtpTransceiverInit
  ) {
    const transceiver = new RTCRtpTransceiver(trackOrKind, init);
    if (this.ontrack && transceiver.receiver.track) {
      this.ontrack(new RTCTrackEvent(transceiver));
    }
    this.transceivers.push(transceiver);
    return transceiver;
  }

  getTransceivers() {
    return this.transceivers;
  }

  createOffer(): RTCSessionDescriptionInit {
    for (const transceiver of this.transceivers) {
      NativeDatachannel.addTransceiver(
        this.pc,
        transceiver.kind,
        transceiver.direction,
        transceiver.sender.track?.id || '',
        transceiver.receiver.track?.id || '',
        'offer'
      );
    }
    const sdp = NativeDatachannel.createOffer(this.pc);
    return { sdp, type: 'offer' };
  }

  createAnswer(): RTCSessionDescriptionInit {
    for (const transceiver of this.transceivers) {
      NativeDatachannel.addTransceiver(
        this.pc,
        transceiver.kind,
        transceiver.direction,
        transceiver.sender.track?.id || '',
        transceiver.receiver.track?.id || '',
        'answer'
      );
    }

    const sdp = NativeDatachannel.createAnswer(this.pc);
    return { sdp, type: 'answer' };
  }

  get localDescription(): string {
    return NativeDatachannel.getLocalDescription(this.pc);
  }

  setLocalDescription(description: RTCSessionDescriptionInit) {
    NativeDatachannel.setLocalDescription(this.pc, description.sdp || '');
  }

  get remoteDescription(): string {
    return NativeDatachannel.getRemoteDescription(this.pc);
  }

  setRemoteDescription(sdp: string) {
    NativeDatachannel.setRemoteDescription(this.pc, sdp);
  }

  setRemoteCandidate(candidate: string, mid: string) {
    NativeDatachannel.addRemoteCandidate(this.pc, candidate, mid);
  }
}
