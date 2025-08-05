import { Text, View, StyleSheet } from 'react-native';
import DatachannelTurboModule from 'react-native-webrtc-turbo';

DatachannelTurboModule.onLocalDescription((obj) => {
  console.log('Local description event received: ' + JSON.stringify(obj));
});

DatachannelTurboModule.onLocalCandidate((obj) => {
  console.log('Local candidate event received: ' + JSON.stringify(obj));
});

const pc = DatachannelTurboModule.createPeerConnection([
  'turn:username:password@host:port',
]);

const videoSdp = `m=video 9 UDP/TLS/RTP/SAVPF 96
a=recvonly
a=rtpmap:96 H264/90000
a=ssrc:42 cname:video
a=mid:0`;
const track = DatachannelTurboModule.addTrack(pc, videoSdp);
DatachannelTurboModule.setLocalDescription(pc, 'offer');

export default function App() {
  return (
    <View style={styles.container}>
      <Text>Track: {track}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
});
