#include "NativeDatachannelModule.h"
#include <jni.h>
#include <string>

namespace facebook::react {
extern "C" {
JNIEXPORT jobject JNICALL Java_com_webrtc_WebrtcFabricManager_getTrackBuffer(
    JNIEnv *env, jobject, jstring trackId) {

	const char *trackIdChars = env->GetStringUTFChars(trackId, nullptr);
	std::string trackIdStr(trackIdChars);
	env->ReleaseStringUTFChars(trackId, trackIdChars);

	std::optional<RGBAFrame> frame = getTrackBuffer(trackIdStr);
	if (!frame.has_value()) {
		return nullptr;
	}

	jbyteArray byteArray = env->NewByteArray(frame->data.size());
	env->SetByteArrayRegion(
	    byteArray, 0, frame->data.size(),
	    reinterpret_cast<const jbyte *>(frame->data.data()));
	jclass RGBAFrameClass = env->FindClass("com/webrtc/RGBAFrame");
	jmethodID constructor =
	    env->GetMethodID(RGBAFrameClass, "<init>", "(III[B)V");
	return env->NewObject(RGBAFrameClass, constructor, frame->width,
	                      frame->height, frame->linesize, byteArray);
}
}
} // namespace facebook::react
