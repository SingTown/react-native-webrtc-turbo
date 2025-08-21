#include "NativeDatachannelModule.h"
#include <jni.h>
#include <string>

namespace facebook::react {
extern "C" {
JNIEXPORT jobject JNICALL Java_com_webrtc_DataChannelLib_getTrackBuffer(
    JNIEnv *env, jobject, jint trackId) {
	std::vector<uint8_t> &buffer = getTrackBuffer(trackId);
	return env->NewDirectByteBuffer(buffer.data(), buffer.size());
}
}
} // namespace facebook::react
