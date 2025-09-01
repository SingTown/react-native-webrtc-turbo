#include "MediaStreamTrack.h"
#include <jni.h>
#include <string>

namespace facebook::react {
extern "C" {
JNIEXPORT jobject JNICALL Java_com_webrtc_WebrtcFabricManager_popVideoStream(
    JNIEnv *env, jobject, jstring id) {

	const char *idChars = env->GetStringUTFChars(id, nullptr);
	std::string idStr(idChars);
	env->ReleaseStringUTFChars(id, idChars);
	if (idStr.empty()) {
		return nullptr;
	}

	auto mediaStreamTrack = getMediaStreamTrack(idStr);
	auto frame = mediaStreamTrack->pop(AV_PIX_FMT_RGBA);
	if (!frame) {
		return nullptr;
	}

	int size = frame->linesize[0] * frame->height;
	jbyteArray byteArray = env->NewByteArray(size);
	env->SetByteArrayRegion(byteArray, 0, size,
	                        reinterpret_cast<const jbyte *>(frame->data[0]));
	jclass RGBAFrameClass = env->FindClass("com/webrtc/RGBAFrame");
	jmethodID constructor =
	    env->GetMethodID(RGBAFrameClass, "<init>", "(III[B)V");
	return env->NewObject(RGBAFrameClass, constructor, frame->width,
	                      frame->height, frame->linesize[0], byteArray);
}
}
} // namespace facebook::react
