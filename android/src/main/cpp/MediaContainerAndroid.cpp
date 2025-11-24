#include "ffmpeg.h"
#include "framepipe.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <chrono>
#include <jni.h>
#include <string>

namespace facebook::react {

extern "C" {

JavaVM *gJvm = nullptr;
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
	gJvm = vm;
	return JNI_VERSION_1_6;
}

JNIEXPORT int JNICALL Java_com_webrtc_WebrtcFabricManager_subscribeVideo(
    JNIEnv *env, jobject, jstring pipeId, jobject surface) {
	if (!surface) {
		throw std::invalid_argument("Surface is null");
	}
	auto scaler = std::make_shared<Scaler>();
	ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
	if (!window) {
		throw std::runtime_error("Failed to get ANativeWindow from Surface");
	}

	auto callback = [window, scaler](std::string, int,
	                                 std::shared_ptr<AVFrame> raw) {
		auto frame =
		    scaler->scale(raw, AV_PIX_FMT_RGBA, raw->width, raw->height);

		ANativeWindow_setBuffersGeometry(window, frame->width, frame->height,
		                                 WINDOW_FORMAT_RGBA_8888);

		ANativeWindow_Buffer buffer;
		if (ANativeWindow_lock(window, &buffer, nullptr) < 0) {
			throw std::runtime_error("Failed to lock ANativeWindow");
		}

		uint8_t *dst = static_cast<uint8_t *>(buffer.bits);
		for (int y = 0; y < frame->height; ++y) {
			uint8_t *srcRow = frame->data[0] + y * frame->linesize[0];
			uint8_t *dstRow = dst + y * buffer.stride * 4;
			memcpy(dstRow, srcRow, frame->width * 4);
		}

		ANativeWindow_unlockAndPost(window);
	};

	auto cleanup = [window](int) { ANativeWindow_release(window); };

	std::string pipeIdStr(env->GetStringUTFChars(pipeId, nullptr));
	return subscribe({pipeIdStr}, callback, cleanup);
}

JNIEXPORT int JNICALL Java_com_webrtc_WebrtcFabricManager_subscribeAudio(
    JNIEnv *env, jobject thiz, jstring pipeId) {
	auto resampler = std::make_shared<Resampler>();
	jobject gFabricManager = env->NewGlobalRef(thiz);

	auto callback = [gFabricManager, resampler](std::string, int,
	                                            std::shared_ptr<AVFrame> raw) {
		JNIEnv *env;
		gJvm->AttachCurrentThread(&env, nullptr);
		auto frame = resampler->resample(raw, AV_SAMPLE_FMT_S16, 48000, 2);
		const jbyte *sample = reinterpret_cast<const jbyte *>(frame->data[0]);
		int length = frame->nb_samples * sizeof(int16_t) * 2;
		jbyteArray byteArray = env->NewByteArray(length);
		env->SetByteArrayRegion(byteArray, 0, length, sample);

		jclass fabricManagerClass = env->GetObjectClass(gFabricManager);
		jfieldID audioTrackField = env->GetFieldID(
		    fabricManagerClass, "audioTrack", "Landroid/media/AudioTrack;");
		jobject audioTrackObj =
		    env->GetObjectField(gFabricManager, audioTrackField);
		jclass audioTrackCls = env->GetObjectClass(audioTrackObj);
		jmethodID writeMethod =
		    env->GetMethodID(audioTrackCls, "write", "([BII)I");
		env->CallIntMethod(audioTrackObj, writeMethod, byteArray, 0, length);
	};

	auto cleanup = [gFabricManager](int) {
		JNIEnv *env;
		gJvm->AttachCurrentThread(&env, nullptr);
		env->DeleteGlobalRef(gFabricManager);
	};

	std::string pipeIdStr(env->GetStringUTFChars(pipeId, nullptr));
	return subscribe({pipeIdStr}, callback, cleanup);
}

JNIEXPORT void JNICALL Java_com_webrtc_WebrtcFabricManager_unsubscribe(
    JNIEnv, jobject, jint subscriptionId) {
	unsubscribe(subscriptionId);
}

JNIEXPORT void JNICALL Java_com_webrtc_Camera_publish(JNIEnv *env, jobject,
                                                      jstring pipeId,
                                                      jobject image) {
	jclass imageClass = env->GetObjectClass(image);
	jmethodID getWidthMethod = env->GetMethodID(imageClass, "getWidth", "()I");
	jmethodID getHeightMethod =
	    env->GetMethodID(imageClass, "getHeight", "()I");
	jint width = env->CallIntMethod(image, getWidthMethod);
	jint height = env->CallIntMethod(image, getHeightMethod);

	jmethodID getPlanesMethod = env->GetMethodID(
	    imageClass, "getPlanes", "()[Landroid/media/Image$Plane;");
	jobjectArray planeArray =
	    (jobjectArray)env->CallObjectMethod(image, getPlanesMethod);

	jobject yPlane = env->GetObjectArrayElement(planeArray, 0);
	jobject uPlane = env->GetObjectArrayElement(planeArray, 1);
	jobject vPlane = env->GetObjectArrayElement(planeArray, 2);
	jclass planeClass = env->GetObjectClass(yPlane);
	jmethodID getBufferMethod =
	    env->GetMethodID(planeClass, "getBuffer", "()Ljava/nio/ByteBuffer;");
	jmethodID getRowStrideMethod =
	    env->GetMethodID(planeClass, "getRowStride", "()I");
	jmethodID getPixelStrideMethod =
	    env->GetMethodID(planeClass, "getPixelStride", "()I");

	jobject yByteBuffer = env->CallObjectMethod(yPlane, getBufferMethod);
	uint8_t *yBufferPtr =
	    static_cast<uint8_t *>(env->GetDirectBufferAddress(yByteBuffer));
	jint yRowStride = env->CallIntMethod(yPlane, getRowStrideMethod);

	jobject uByteBuffer = env->CallObjectMethod(uPlane, getBufferMethod);
	uint8_t *uBufferPtr =
	    static_cast<uint8_t *>(env->GetDirectBufferAddress(uByteBuffer));
	jint uRowStride = env->CallIntMethod(uPlane, getRowStrideMethod);
	jint uPixelStride = env->CallIntMethod(uPlane, getPixelStrideMethod);

	jobject vByteBuffer = env->CallObjectMethod(vPlane, getBufferMethod);
	uint8_t *vBufferPtr =
	    static_cast<uint8_t *>(env->GetDirectBufferAddress(vByteBuffer));
	jint vRowStride = env->CallIntMethod(vPlane, getRowStrideMethod);
	jint vPixelStride = env->CallIntMethod(vPlane, getPixelStrideMethod);

	auto frame = createVideoFrame(AV_PIX_FMT_NV12, width, height);
	// Copy Y
	for (int y = 0; y < height; ++y) {
		memcpy(frame->data[0] + y * frame->linesize[0],
		       yBufferPtr + y * yRowStride, width);
	}

	// Copy UV
	for (int y = 0; y < height / 2; ++y) {
		for (int x = 0; x < width / 2; ++x) {
			frame->data[1][y * frame->linesize[1] + x * 2] =
			    uBufferPtr[y * uRowStride + x * uPixelStride];
			frame->data[1][y * frame->linesize[1] + x * 2 + 1] =
			    vBufferPtr[y * vRowStride + x * vPixelStride];
		}
	}

	env->DeleteLocalRef(yPlane);
	env->DeleteLocalRef(uPlane);
	env->DeleteLocalRef(vPlane);
	env->DeleteLocalRef(planeArray);
	env->DeleteLocalRef(imageClass);
	env->DeleteLocalRef(planeClass);

	std::string pipeIdStr(env->GetStringUTFChars(pipeId, nullptr));
	publish(pipeIdStr, frame);
}

JNIEXPORT void JNICALL Java_com_webrtc_Microphone_publish(
    JNIEnv *env, jobject, jstring pipeId, jbyteArray audioBuffer, jint size) {

	auto frame =
	    createAudioFrame(AV_SAMPLE_FMT_S16, 48000, 1, size / sizeof(int16_t));
	jboolean isCopy = JNI_FALSE;
	jbyte *audioData = env->GetByteArrayElements(audioBuffer, &isCopy);
	memcpy(frame->data[0], audioData, size);
	env->ReleaseByteArrayElements(audioBuffer, audioData, JNI_ABORT);

	std::string pipeIdStr(env->GetStringUTFChars(pipeId, nullptr));
	publish(pipeIdStr, frame);
}
}
} // namespace facebook::react
