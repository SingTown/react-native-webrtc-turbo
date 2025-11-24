import { useState, useEffect } from 'react';
import { Button, StyleSheet, SafeAreaView, View, Alert } from 'react-native';
import { PermissionsAndroid, Platform } from 'react-native';
import RNFS from 'react-native-fs';
import { CameraRoll } from '@react-native-camera-roll/camera-roll';
import path from 'path-browserify';

import {
  WebrtcView,
  MediaStream,
  MediaDevices,
  MediaRecorder,
} from 'react-native-webrtc-turbo';

async function requestPermission() {
  if (Platform.OS === 'android') {
    const granted = await PermissionsAndroid.request(
      PermissionsAndroid.PERMISSIONS.READ_MEDIA_IMAGES
    );
    if (granted !== PermissionsAndroid.RESULTS.GRANTED) {
      throw new Error('Permission denied');
    }
  }
}

export default function Camera() {
  const [stream, setStream] = useState<MediaStream | null>(null);
  const mp4path = path.join(RNFS.TemporaryDirectoryPath, 'test_recording.mp4');
  const pngPath = path.join(RNFS.TemporaryDirectoryPath, 'test_photo.png');
  const [recording, setRecording] = useState<MediaRecorder | null>(null);

  useEffect(() => {
    let localStream: MediaStream | null = null;
    (async () => {
      try {
        localStream = await MediaDevices.getUserMedia({
          video: true,
          audio: true,
        });
        setStream(localStream);
      } catch (e) {
        Alert.alert('Permission Error');
        throw e;
      }
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
        <WebrtcView style={styles.player} stream={stream} resizeMode="fill" />
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
        <View style={styles.buttonContainer}>
          <Button
            title="Take Photo"
            onPress={async () => {
              requestPermission();
              let new_recording = new MediaRecorder(stream!);
              console.log('Taking photo...', pngPath);
              await new_recording.takePhoto(pngPath);
              console.log('new_recording.takePhoto done');
              await CameraRoll.save(pngPath, { type: 'photo' });
              console.log('CameraRoll.save');
            }}
          />
        </View>
        <View style={styles.buttonContainer}>
          <Button
            title="Start Recording"
            onPress={async () => {
              if (recording) {
                return;
              }
              requestPermission();
              let new_recording = new MediaRecorder(stream!);
              new_recording.startRecording(mp4path);
              setRecording(new_recording);
            }}
          />
        </View>
        <View style={styles.buttonContainer}>
          <Button
            title="Stop Recording"
            onPress={async () => {
              if (!recording) {
                return;
              }
              recording.stopRecording();
              await CameraRoll.save(mp4path, { type: 'video' });
              console.log('Saved recording to camera roll');
              setRecording(null);
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
    height: 240,
  },
  buttonContainer: {
    height: 44,
    margin: 5,
    justifyContent: 'center',
    backgroundColor: '#a8a4a4a3',
  },
});
