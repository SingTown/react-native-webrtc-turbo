import {
  Button,
  Text,
  StyleSheet,
  TextInput,
  SafeAreaView,
  View,
  ScrollView,
} from 'react-native';
import { useState, useEffect } from 'react';
import { RTCPeerConnection } from 'react-native-webrtc-turbo';
import Clipboard from '@react-native-clipboard/clipboard';

const iceServers = [
  {
    urls: 'turn:turn.example.com:3478',
    username: 'example',
    credential: 'example123',
  },
];

export default function App() {
  const [pc, setPc] = useState<RTCPeerConnection | null>(null);
  const [localDescription, setLocalDescription] = useState('');
  const [localCandidates, setLocalCandidates] = useState('');
  const [inputRemoteSDP, setInputRemoteSDP] = useState('');
  const [inputCandidates, setInputCandidates] = useState('');

  useEffect(() => {
    console.log('Setting up local description listener');
    const peerconnection = new RTCPeerConnection({
      iceServers: iceServers,
    });
    setPc(peerconnection);
    peerconnection.onicecandidate = (candidate) => {
      setLocalCandidates((prev) => prev + candidate.trim() + '\n');
    };

    const videoSdp = `m=video 9 UDP/TLS/RTP/SAVPF 96
a=recvonly
a=rtpmap:96 H264/90000
a=ssrc:42 cname:video
a=mid:0`;
    const tr = peerconnection.addTrack(videoSdp);
    tr.onMessage((message) => {
      console.log('Received message on track:', message.byteLength);
    });

    const offer = peerconnection.createOffer();
    peerconnection.setLocalDescription(offer);
    const sdp = peerconnection.localDescription;
    setLocalDescription(sdp);
    return () => {
      peerconnection.close();
    };
  }, []);

  return (
    <SafeAreaView style={styles.safeArea}>
      <View style={styles.container}>
        <View style={styles.section}>
          <ScrollView>
            <Text selectable={true} style={styles.text}>
              {localDescription || 'NO'}
            </Text>
          </ScrollView>
          <View style={styles.buttonContainer}>
            <Button
              title="Copy Local SDP"
              onPress={() => {
                Clipboard.setString(localDescription);
              }}
            />
          </View>
        </View>

        <View style={styles.section}>
          <ScrollView>
            <Text selectable={true} style={styles.text}>
              {localCandidates || 'NO'}
            </Text>
          </ScrollView>
          <View style={styles.buttonContainer}>
            <Button
              title="Copy Local Candidates"
              onPress={() => {
                Clipboard.setString(localCandidates);
              }}
            />
          </View>
        </View>

        <View style={styles.section}>
          <TextInput
            style={styles.textInput}
            value={inputRemoteSDP}
            onChangeText={setInputRemoteSDP}
            placeholder="Please enter remote SDP..."
            multiline
          />
          <View style={styles.buttonContainer}>
            <Button
              title="Set Remote SDP"
              onPress={() => {
                pc?.setRemoteDescription(
                  inputRemoteSDP.trim() + '\n',
                  'answer'
                );
              }}
            />
          </View>
        </View>

        <View style={styles.section}>
          <TextInput
            style={styles.textInput}
            value={inputCandidates}
            onChangeText={setInputCandidates}
            placeholder="Please enter remote candidates..."
            multiline
          />
          <View style={styles.buttonContainer}>
            <Button
              title="Add Remote Candidate"
              onPress={() => {
                for (const remoteCandidate of inputCandidates.split('\n')) {
                  if (remoteCandidate.trim() === '') continue;
                  pc?.setRemoteCandidate(remoteCandidate, '0');
                }
              }}
            />
          </View>
        </View>
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
  },
  container: {
    padding: 10,
    flexGrow: 1,
    gap: 20,
  },
  section: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#e9ecef',
    justifyContent: 'space-between',
  },
  textInput: {
    flex: 1,
    width: '100%',
  },
  text: {
    flex: 1,
    minHeight: 40,
    maxHeight: 200,
  },
  buttonContainer: {
    height: 44,
    justifyContent: 'center',
    backgroundColor: '#a8a4a4a3',
  },
});
