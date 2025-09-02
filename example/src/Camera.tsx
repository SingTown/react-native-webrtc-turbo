import { useState, useEffect } from 'react';
import {
  WebrtcView,
  MediaStream,
  MediaDevices,
} from 'react-native-webrtc-turbo';

import { StyleSheet, SafeAreaView, View } from 'react-native';

export default function Camera() {
  const [stream, setStream] = useState<MediaStream | null>(null);

  useEffect(() => {
    (async () => {
      const mediaStream = await MediaDevices.getUserMedia({
        video: true,
        audio: false,
      });
      setStream(mediaStream);
    })();
  }, []);

  return (
    <SafeAreaView style={styles.safeArea}>
      <View style={styles.container}>
        <WebrtcView style={styles.player} stream={stream} />
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
});
