import { useState, useEffect } from 'react';
import Clipboard from '@react-native-clipboard/clipboard';
import AsyncStorage from '@react-native-async-storage/async-storage';
import {
  WebrtcView,
  RTCPeerConnection,
  MediaStream,
  MediaDevices,
  RTCIceCandidate,
} from 'react-native-webrtc-turbo';

import {
  Button,
  Text,
  StyleSheet,
  TextInput,
  SafeAreaView,
  View,
  ScrollView,
} from 'react-native';
import Tabs from './Tabs';

export default function App() {
  const [activeTab, setActiveTab] = useState('RemoteSDP');
  const [pc, setPc] = useState<RTCPeerConnection | null>(null);
  const [stream, setStream] = useState<MediaStream | null>(null);
  const [localDescription, setLocalDescription] = useState('');
  const [localCandidates, setLocalCandidates] = useState('');
  const [inputRemoteSDP, setInputRemoteSDP] = useState('');
  const [inputCandidates, setInputCandidates] = useState('');

  useEffect(() => {
    let peerconnection: RTCPeerConnection | null = null;
    let localStream: MediaStream | null = null;
    let remoteStream: MediaStream | null = null;
    (async () => {
      const url = await AsyncStorage.getItem('iceUrl');
      const user = await AsyncStorage.getItem('iceUsername');
      const pwd = await AsyncStorage.getItem('icePassword');
      peerconnection = new RTCPeerConnection({
        iceServers: [
          {
            urls: url || '',
            username: user || '',
            credential: pwd || '',
          },
        ],
      });

      setPc(peerconnection);

      let tempLocalCandidates: RTCIceCandidate[] = [];
      peerconnection.onicecandidate = (event) => {
        const candidate = event.candidate;
        if (!candidate) {
          setLocalCandidates(JSON.stringify(tempLocalCandidates, null, 2));
          return;
        }
        tempLocalCandidates.push(candidate);
      };

      peerconnection.ontrack = (event) => {
        const s = event.streams[0];
        if (s) {
          remoteStream = s;
          setStream(remoteStream);
        }
      };

      localStream = await MediaDevices.getUserMedia({
        video: true,
        audio: true,
      });

      localStream?.getTracks().forEach((track) => {
        peerconnection?.addTransceiver(track, {
          direction: 'sendrecv',
          streams: [localStream!],
        });
      });
    })();

    return () => {
      localStream?.getTracks().forEach((track) => {
        track.stop();
      });
      remoteStream?.getTracks().forEach((track) => {
        track.stop();
      });
      setStream(null);

      peerconnection?.close();
      setPc(null);
    };
  }, []);

  return (
    <SafeAreaView style={styles.safeArea}>
      <View style={styles.container}>
        <WebrtcView style={styles.player} stream={stream} />
        <Tabs activeTab={activeTab} setActiveTab={setActiveTab} />

        {activeTab === 'LocalSDP' ? (
          <View style={styles.section}>
            <ScrollView>
              <Text selectable={true}>{localDescription}</Text>
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
        ) : null}

        {activeTab === 'LocalCand' ? (
          <View style={styles.section}>
            <ScrollView>
              <Text selectable={true}>{localCandidates}</Text>
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
        ) : null}

        {activeTab === 'RemoteSDP' ? (
          <View style={styles.section}>
            <ScrollView>
              <TextInput
                style={styles.multilineInput}
                value={inputRemoteSDP}
                onChangeText={setInputRemoteSDP}
                placeholder="Please enter remote SDP..."
                multiline
              />
            </ScrollView>
            <View style={styles.buttonContainer}>
              <Button
                title="Set Remote SDP"
                onPress={async () => {
                  if (!pc) return;
                  pc.setRemoteDescription({
                    sdp: inputRemoteSDP.trim() + '\n',
                    type: 'offer',
                  });
                  const answer = await pc.createAnswer();
                  setLocalDescription(answer.sdp || '');
                  pc.setLocalDescription(answer);
                }}
              />
            </View>
          </View>
        ) : null}

        {activeTab === 'RemoteCand' ? (
          <View style={styles.section}>
            <ScrollView>
              <TextInput
                style={styles.multilineInput}
                value={inputCandidates}
                onChangeText={setInputCandidates}
                placeholder="Please enter remote candidates..."
                multiline
              />
            </ScrollView>
            <View style={styles.buttonContainer}>
              <Button
                title="Add Remote Candidate"
                onPress={async () => {
                  for (const candidate of JSON.parse(inputCandidates)) {
                    await pc?.addIceCandidate(candidate);
                  }
                }}
              />
            </View>
          </View>
        ) : null}
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
    gap: 10,
  },
  player: {
    height: 300,
  },
  section: {
    flex: 1,
    borderWidth: 1,
    borderColor: 'tomato',
    justifyContent: 'space-between',
  },
  multilineInput: {
    flex: 1,
    width: '100%',
  },
  buttonContainer: {
    height: 44,
    margin: 5,
    justifyContent: 'center',
    backgroundColor: '#a8a4a4a3',
  },
});
