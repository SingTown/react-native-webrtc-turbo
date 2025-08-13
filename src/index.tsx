import DatachannelTurboModule from './NativeDatachannelTurboModule';
import type { EventSubscription } from 'react-native';

export type RTCSdpType = 'answer' | 'offer' | 'pranswer' | 'rollback';
export interface RTCSessionDescriptionInit {
  sdp?: string;
  type: RTCSdpType;
}

function convertIceServersToUrls(iceServers: RTCIceServer[]): string[] {
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

interface RTCIceServer {
  credential?: string;
  urls: string[] | string;
  username?: string;
}

interface RTCConfiguration {
  iceServers?: RTCIceServer[];
}

export class RTCPeerConnection {
  private pc: number;
  private onLocalCandidateCallback: EventSubscription;

  public onicecandidate: ((candidate: string) => void) | null = null;

  constructor(configuration?: RTCConfiguration) {
    const servers = convertIceServersToUrls(configuration?.iceServers || []);
    this.pc = DatachannelTurboModule.createPeerConnection(servers);
    this.onLocalCandidateCallback = DatachannelTurboModule.onLocalCandidate(
      (obj) => {
        if (obj.pc !== this.pc || !this.onicecandidate) {
          return;
        }
        this.onicecandidate(obj.candidate);
      }
    );
  }

  get localDescription(): string {
    return DatachannelTurboModule.getLocalDescription(this.pc);
  }

  close() {
    this.onLocalCandidateCallback.remove();
    DatachannelTurboModule.closePeerConnection(this.pc);
  }

  createOffer(): RTCSessionDescriptionInit {
    const sdp = DatachannelTurboModule.createOffer(this.pc);
    return { sdp, type: 'offer' };
  }

  addTrack(sdp: string) {
    const tr = DatachannelTurboModule.addTrack(this.pc, sdp);
    return new RTCTrack(tr);
  }

  setLocalDescription(description: RTCSessionDescriptionInit) {
    DatachannelTurboModule.setLocalDescription(this.pc, description.type);
  }

  setRemoteDescription(sdp: string, type: string) {
    DatachannelTurboModule.setRemoteDescription(this.pc, sdp, type);
  }

  setRemoteCandidate(candidate: string, mid: string) {
    DatachannelTurboModule.addRemoteCandidate(this.pc, candidate, mid);
  }
}

export class RTCTrack {
  private tr: number;

  constructor(tr: number) {
    this.tr = tr;
  }

  onMessage(callback: (message: ArrayBuffer) => void) {
    return DatachannelTurboModule.onMessage((obj) => {
      if (obj.id !== this.tr) {
        return;
      }
      const arrayBuffer = Uint8Array.from(atob(obj.base64), (c) =>
        c.charCodeAt(0)
      ).buffer;

      callback(arrayBuffer);
    });
  }

  close() {
    DatachannelTurboModule.deleteTrack(this.tr);
  }
}
