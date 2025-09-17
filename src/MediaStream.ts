import { MediaStreamTrack } from './MediaStreamTrack';

export class MediaStream {
  public msid: string;
  private tracks: MediaStreamTrack[] = [];

  constructor(msid: string) {
    this.msid = msid;
  }

  getTracks(): MediaStreamTrack[] {
    return this.tracks;
  }

  addTrack(track: MediaStreamTrack) {
    this.tracks.push(track);
  }

  removeTrack(track: MediaStreamTrack) {
    this.tracks = this.tracks.filter((t) => t.id !== track.id);
  }

  getTrackById(id: string): MediaStreamTrack | null {
    const track = this.tracks.find((t) => t.id === id);
    return track || null;
  }

  getAudioTracks(): MediaStreamTrack[] {
    return this.tracks.filter((t) => t.kind === 'audio');
  }

  getVideoTracks(): MediaStreamTrack[] {
    return this.tracks.filter((t) => t.kind === 'video');
  }
}
