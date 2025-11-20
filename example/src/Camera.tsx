import { useState, useEffect } from 'react';
import { Button } from 'react-native';

import {
  WebrtcView,
  MediaStream,
  MediaDevices,
} from 'react-native-webrtc-turbo';

import { StyleSheet, SafeAreaView, View } from 'react-native';

export default function Camera() {
  const [stream, setStream] = useState<MediaStream | null>(null);

  useEffect(() => {
    let localStream: MediaStream | null = null;
    (async () => {
      localStream = await MediaDevices.getUserMedia({
        video: true,
        audio: true,
      });
      setStream(localStream);
    })();

    return () => {
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
        <View style={styles.buttonContainer}>
          <Button
            title="Enable Video"
            onPress={() => {
              stream?.getVideoTracks().forEach((track) => {
                track.enabled = true;
              });
            }}
          />
        </View>
        <View style={styles.buttonContainer}>
          <Button
            title="Disable Video"
            onPress={() => {
              stream?.getVideoTracks().forEach((track) => {
                track.enabled = false;
              });
            }}
          />
        </View>
        <View style={styles.buttonContainer}>
          <Button
            title="Enable Audio"
            onPress={() => {
              stream?.getAudioTracks().forEach((track) => {
                track.enabled = true;
              });
            }}
          />
        </View>
        <View style={styles.buttonContainer}>
          <Button
            title="Disable Audio"
            onPress={() => {
              stream?.getAudioTracks().forEach((track) => {
                track.enabled = false;
              });
            }}
          />
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
    gap: 10,
  },
  player: {
    height: 300,
  },
  buttonContainer: {
    height: 44,
    margin: 5,
    justifyContent: 'center',
    backgroundColor: '#a8a4a4a3',
  },
});
