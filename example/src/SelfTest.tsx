import { useState, useEffect } from 'react';
import Clipboard from '@react-native-clipboard/clipboard';
import AsyncStorage from '@react-native-async-storage/async-storage';
import {
  WebrtcView,
  RTCPeerConnection,
  MediaStream,
  MediaDevices,
} from 'react-native-webrtc-turbo';

import {
  Button,
  Text,
  StyleSheet,
  SafeAreaView,
  View,
  ScrollView,
} from 'react-native';
import Tabs from './Tabs';

export default function SelfTest() {
  const [activeTab, setActiveTab] = useState('LocalSDP');
  const [stream, setStream] = useState<MediaStream | null>(null);
  const [localDescription, setLocalDescription] = useState('');
  const [localCandidates, setLocalCandidates] = useState('');
  const [remoteDescription, setRemoteDescription] = useState('');
  const [remoteCandidates, setRemoteCandidates] = useState('');

  useEffect(() => {
    let peerconnection1: RTCPeerConnection | null = null;
    let peerconnection2: RTCPeerConnection | null = null;
    let localStream: MediaStream | null = null;

    (async () => {
      const url = await AsyncStorage.getItem('iceUrl');
      const user = await AsyncStorage.getItem('iceUsername');
      const pwd = await AsyncStorage.getItem('icePassword');
      peerconnection1 = new RTCPeerConnection({
        iceServers: [
          {
            urls: url || '',
            username: user || '',
            credential: pwd || '',
          },
        ],
      });
      peerconnection2 = new RTCPeerConnection({
        iceServers: [
          {
            urls: url || '',
            username: user || '',
            credential: pwd || '',
          },
        ],
      });

      setLocalCandidates('');
      peerconnection1.onicecandidate = (candidate) => {
        peerconnection2!.addIceCandidate({
          candidate: candidate,
          sdpMid: '0',
        });
        setLocalCandidates((prev) => prev + candidate.trim() + '\n');
      };

      peerconnection2.onicecandidate = (candidate) => {
        // peerconnection1.addIceCandidate({
        //   candidate: candidate,
        //   sdpMid: '0',
        // });
        setRemoteCandidates((prev) => prev + candidate.trim() + '\n');
      };

      setRemoteCandidates('');

      peerconnection1.ontrack = (event) => {
        const s = event.streams[0];
        s && setStream(s);
      };

      peerconnection1.addTransceiver('video', {
        direction: 'recvonly',
      });
      peerconnection1.addTransceiver('audio', {
        direction: 'recvonly',
      });

      const offer = await peerconnection1.createOffer();
      peerconnection1.setLocalDescription(offer);
      peerconnection2.setRemoteDescription(offer);
      setLocalDescription(offer.sdp || '');

      localStream = await MediaDevices.getUserMedia({
        video: true,
        audio: true,
      });

      localStream.getTracks().forEach((track) => {
        peerconnection2!.addTransceiver(track, {
          direction: 'sendonly',
          streams: [localStream!],
        });
      });

      const answer = await peerconnection2.createAnswer();
      peerconnection1.setRemoteDescription(answer);
      setRemoteDescription(answer.sdp || '');
    })();

    return () => {
      peerconnection1?.close();
      peerconnection2?.close();
      peerconnection1 = null;
      peerconnection2 = null;
      localStream?.getTracks().forEach((track) => {
        track.stop();
      });
      setStream(null);
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
            <Text>{remoteDescription}</Text>
            <View style={styles.buttonContainer}>
              <Button
                title="Set Remote SDP"
                onPress={() => {
                  Clipboard.setString(remoteDescription);
                }}
              />
            </View>
          </View>
        ) : null}

        {activeTab === 'RemoteCand' ? (
          <View style={styles.section}>
            <Text> {remoteCandidates} </Text>
            <View style={styles.buttonContainer}>
              <Button
                title="Copy Remote Candidates"
                onPress={async () => {
                  Clipboard.setString(remoteCandidates);
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
  buttonContainer: {
    height: 44,
    margin: 5,
    justifyContent: 'center',
    backgroundColor: '#a8a4a4a3',
  },
});
