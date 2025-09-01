import { useState, useEffect } from 'react';
import Clipboard from '@react-native-clipboard/clipboard';
import AsyncStorage from '@react-native-async-storage/async-storage';
import {
  WebrtcView,
  RTCPeerConnection,
  MediaStream,
  NativeMediaDevice,
} from 'react-native-webrtc-turbo';
import type { RTCIceServer } from 'react-native-webrtc-turbo';
import {
  Button,
  Text,
  StyleSheet,
  TextInput,
  SafeAreaView,
  View,
  ScrollView,
  Alert,
} from 'react-native';
import Tabs from './Tabs';

export default function App() {
  const [activeTab, setActiveTab] = useState('ICEServer');
  const [pc, setPc] = useState<RTCPeerConnection | null>(null);
  const [stream, setStream] = useState<MediaStream | null>(null);
  const [localDescription, setLocalDescription] = useState('');
  const [localCandidates, setLocalCandidates] = useState('');
  const [inputRemoteSDP, setInputRemoteSDP] = useState('');
  const [inputCandidates, setInputCandidates] = useState('');

  const [iceUrl, setIceUrl] = useState('turn:turn.example.com:3478');
  const [iceUsername, setIceUsername] = useState('username');
  const [icePassword, setIcePassword] = useState('password');

  const initializeCamera = async () => {
    try {
      const hasPermission = await NativeMediaDevice.requestCameraPermission();

      if (hasPermission) {
        // 权限已获得，创建摄像头
        const cameraResult = await NativeMediaDevice.createCamera('0');
        console.log('Camera created:', cameraResult);
        Alert.alert('Success', 'Camera initialized successfully');
      } else {
        // 权限被拒绝
        Alert.alert(
          'Permission Required',
          'Camera permission is required for video capture. Please grant permission in Settings if needed.',
          [{ text: 'OK', style: 'default' }]
        );
      }
    } catch (error) {
      console.error('Camera initialization failed:', error);
      Alert.alert('Error', `Failed to initialize camera: ${error}`);
    }
  };

  useEffect(() => {
    (async () => {
      const url = await AsyncStorage.getItem('iceUrl');
      const user = await AsyncStorage.getItem('iceUsername');
      const pwd = await AsyncStorage.getItem('icePassword');
      if (url) setIceUrl(url);
      if (user) setIceUsername(user);
      if (pwd) setIcePassword(pwd);
    })();
  }, []);

  function Offer(server: RTCIceServer) {
    console.log('Setting up local description listener');
    const peerconnection = new RTCPeerConnection({
      iceServers: [server],
    });
    setPc(peerconnection);
    setLocalCandidates('');
    peerconnection.onicecandidate = (candidate) => {
      setLocalCandidates((prev) => prev + candidate.trim() + '\n');
    };
    peerconnection.ontrack = (event) => {
      const s = event.streams[0];
      s && setStream(s);
    };

    peerconnection.addTransceiver('video', {
      direction: 'recvonly',
    });

    const offer = peerconnection.createOffer();
    peerconnection.setLocalDescription(offer);
    const sdp = peerconnection.localDescription;
    setLocalDescription(sdp);
  }

  return (
    <SafeAreaView style={styles.safeArea}>
      <View style={styles.container}>
        <WebrtcView style={styles.player} stream={stream} />
        <Tabs activeTab={activeTab} setActiveTab={setActiveTab} />

        {activeTab === 'ICEServer' ? (
          <View style={styles.section}>
            <ScrollView>
              <View>
                <TextInput
                  style={styles.textInput}
                  value={iceUrl}
                  onChangeText={(text) => {
                    setIceUrl(text);
                    AsyncStorage.setItem('iceUrl', text);
                  }}
                />
                <TextInput
                  style={styles.textInput}
                  value={iceUsername}
                  onChangeText={(text) => {
                    setIceUsername(text);
                    AsyncStorage.setItem('iceUsername', text);
                  }}
                />
                <TextInput
                  style={styles.textInput}
                  value={icePassword}
                  onChangeText={(text) => {
                    setIcePassword(text);
                    AsyncStorage.setItem('icePassword', text);
                  }}
                  secureTextEntry
                />
              </View>
            </ScrollView>
            <View style={styles.buttonContainer}>
              <Button title="Initialize Camera" onPress={initializeCamera} />
            </View>
            <View style={styles.buttonContainer}>
              <Button
                title="Offer"
                onPress={() => {
                  Offer({
                    urls: iceUrl,
                    username: iceUsername,
                    credential: icePassword,
                  });
                }}
              />
            </View>
            <View style={styles.buttonContainer}>
              <Button
                title="Answer"
                onPress={() => {
                  Clipboard.setString(localCandidates);
                }}
              />
            </View>
          </View>
        ) : null}

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
            <TextInput
              style={styles.multilineInput}
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
        ) : null}

        {activeTab === 'RemoteCand' ? (
          <View style={styles.section}>
            <TextInput
              style={styles.multilineInput}
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
  textInput: {
    padding: 10,
    borderWidth: 1,
    borderColor: 'tomato',
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
