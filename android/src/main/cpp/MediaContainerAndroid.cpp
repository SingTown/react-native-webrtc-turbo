#include "MediaContainer.h"
#include <android/bitmap.h>
#include <chrono>
#include <jni.h>
#include <string>

namespace facebook::react {

std::shared_ptr<VideoContainer> getVideoContainerJni(JNIEnv *env, jstring id) {
	const char *idChars = env->GetStringUTFChars(id, nullptr);
	std::string idStr(idChars);
	env->ReleaseStringUTFChars(id, idChars);
	if (idStr.empty()) {
		return nullptr;
	}
	return getVideoContainer(idStr);
}

std::shared_ptr<AudioContainer> getAudioContainerJni(JNIEnv *env, jstring id) {
	const char *idChars = env->GetStringUTFChars(id, nullptr);
	std::string idStr(idChars);
	env->ReleaseStringUTFChars(id, idChars);
	if (idStr.empty()) {
		return nullptr;
	}
	return getAudioContainer(idStr);
}

extern "C" {

JNIEXPORT jobject JNICALL
Java_com_webrtc_WebrtcFabricManager_getFrame(JNIEnv *env, jobject, jstring id) {
	auto container = getVideoContainerJni(env, id);
	if (!container) {
		return nullptr;
	}
	auto frame = container->popVideo(AV_PIX_FMT_RGBA);
	if (!frame) {
		return nullptr;
	}

	jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
	jfieldID argb8888FieldID = env->GetStaticFieldID(
	    bitmapConfigClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
	jobject bitmapConfig =
	    env->GetStaticObjectField(bitmapConfigClass, argb8888FieldID);
	jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
	jmethodID createBitmapMethodID = env->GetStaticMethodID(
	    bitmapClass, "createBitmap",
	    "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
	jobject bitmap =
	    env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID,
	                                frame->width, frame->height, bitmapConfig);
	AndroidBitmapInfo bitmapInfo;
	int result = AndroidBitmap_getInfo(env, bitmap, &bitmapInfo);
	if (result != ANDROID_BITMAP_RESULT_SUCCESS) {
		throw std::runtime_error("AndroidBitmap_getInfo failed");
	}

	void *pixels;
	result = AndroidBitmap_lockPixels(env, bitmap, &pixels);
	if (result != ANDROID_BITMAP_RESULT_SUCCESS) {
		throw std::runtime_error("AndroidBitmap_lockPixels failed");
	}

	for (int y = 0; y < frame->height; ++y) {
		uint8_t *srcRow = frame->data[0] + y * frame->linesize[0];
		uint8_t *dstRow =
		    static_cast<uint8_t *>(pixels) + y * bitmapInfo.stride;
		memcpy(dstRow, srcRow, frame->width * 4);
	}
	AndroidBitmap_unlockPixels(env, bitmap);
	env->DeleteLocalRef(bitmapConfigClass);
	env->DeleteLocalRef(bitmapClass);
	env->DeleteLocalRef(bitmapConfig);
	return bitmap;
}
JNIEXPORT void JNICALL Java_com_webrtc_Camera_processFrame(JNIEnv *env, jobject,
                                                           jstring id,
                                                           jobject image) {

	auto container = getVideoContainerJni(env, id);
	if (!container) {
		return;
	}

	static jlong baseTimestamp = 0;
	static bool isFirstFrame = true;

	jclass imageClass = env->GetObjectClass(image);
	jmethodID getWidthMethod = env->GetMethodID(imageClass, "getWidth", "()I");
	jmethodID getHeightMethod =
	    env->GetMethodID(imageClass, "getHeight", "()I");
	jmethodID getTimestampMethod =
	    env->GetMethodID(imageClass, "getTimestamp", "()J");
	jint width = env->CallIntMethod(image, getWidthMethod);
	jint height = env->CallIntMethod(image, getHeightMethod);
	jlong timestamp = env->CallLongMethod(image, getTimestampMethod);

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

	if (isFirstFrame) {
		baseTimestamp = timestamp;
		isFirstFrame = false;
	}

	int pts = (timestamp - baseTimestamp) * 9 / 100000;
	auto frame = createVideoFrame(AV_PIX_FMT_NV12, pts, width, height);
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

	container->push(frame);
}

JNIEXPORT jbyteArray JNICALL Java_com_webrtc_Sound_getFrame(JNIEnv *env,
                                                              jobject,
                                                              jstring id) {
	auto container = getAudioContainerJni(env, id);
	if (!container) {
		return nullptr;
	}
	auto frame = container->popAudio(AV_SAMPLE_FMT_S16, 48000, 2);
	if (!frame) {
		return nullptr;
	}

	const jbyte *sample = reinterpret_cast<const jbyte *>(frame->data[0]);
	int length = frame->nb_samples * sizeof(int16_t) * 2;
	jbyteArray byteArray = env->NewByteArray(length);
	env->SetByteArrayRegion(byteArray, 0, length, sample);

	return byteArray;
}

JNIEXPORT void JNICALL Java_com_webrtc_Microphone_processFrame(
    JNIEnv *env, jobject, jstring id, jbyteArray audioBuffer, jint size) {
	static bool isFirstFrame = true;
	static auto baseTimestamp = std::chrono::system_clock::now();

	auto container = getAudioContainerJni(env, id);
	if (!container) {
		return;
	}

	auto now = std::chrono::system_clock::now();
	if (isFirstFrame) {
		baseTimestamp = now;
		isFirstFrame = false;
	}

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
	              now - baseTimestamp)
	              .count();

	auto frame = createAudioFrame(AV_SAMPLE_FMT_S16, ms * 48, 48000, 1,
	                              size / sizeof(int16_t));
	jboolean isCopy = JNI_FALSE;
	jbyte *audioData = env->GetByteArrayElements(audioBuffer, &isCopy);
	memcpy(frame->data[0], audioData, size);
	env->ReleaseByteArrayElements(audioBuffer, audioData, JNI_ABORT);

	container->push(frame);
}
}
} // namespace facebook::react
