import { View, StyleSheet, Text } from 'react-native';
import { WebrtcView, NativeDatachannelModule } from 'react-native-webrtc';

const result = NativeDatachannelModule.reverseString('Hello World');

export default function App() {
  return (
    <View style={styles.container}>
      <Text>{result}</Text>
      <WebrtcView trackId={1} style={styles.box} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  box: {
    width: 60,
    height: 60,
    marginVertical: 20,
  },
});
