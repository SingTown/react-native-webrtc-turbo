#include "MediaStreamTrack.h"
#include <android/bitmap.h>
#include <android/log.h>
#include <jni.h>
#include <string>

#define LOG_TAG "Webrtc"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace facebook::react {
extern "C" {
JNIEXPORT jobject JNICALL
Java_com_webrtc_WebrtcFabricManager_popVideoStreamTrack(JNIEnv *env, jobject,
                                                        jstring id) {

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
JNIEXPORT void JNICALL Java_com_webrtc_Camera_pushVideoStreamTrack(
    JNIEnv *env, jobject, jstring id, jobject image) {

	const char *idChars = env->GetStringUTFChars(id, nullptr);
	std::string idStr(idChars);
	env->ReleaseStringUTFChars(id, idChars);
	if (idStr.empty()) {
		return;
	}
	auto mediaStreamTrack = getMediaStreamTrack(idStr);

	// LOGE("Java_com_webrtc_Camera_pushVideoStreamTrack: %s\n", idStr.c_str());

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
	jint yPixelStride = env->CallIntMethod(yPlane, getPixelStrideMethod);

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

	auto frame = createAVFrame();
	frame->width = (int)width;
	frame->height = (int)height;
	frame->format = AV_PIX_FMT_YUV420P;
	// frame->pts = pts_90k - self.ptsBase;

	int ret = av_frame_get_buffer(frame.get(), 32);
	if (ret < 0) {
		throw std::runtime_error("Could not allocate image");
	}

	// Copy Y
	for (int y = 0; y < height; ++y) {
		memcpy(frame->data[0] + y * frame->linesize[0],
		       yBufferPtr + y * yRowStride, width);
	}

	// Copy U
	for (int y = 0; y < height / 2; ++y) {
		for (int x = 0; x < width / 2; ++x) {
			frame->data[1][y * frame->linesize[1] + x] =
			    uBufferPtr[y * uRowStride + x * uPixelStride];
		}
	}
	// Copy V
	for (int y = 0; y < height / 2; ++y) {
		for (int x = 0; x < width / 2; ++x) {
			frame->data[2][y * frame->linesize[2] + x] =
			    vBufferPtr[y * vRowStride + x * vPixelStride];
		}
	}

	env->DeleteLocalRef(yPlane);
	env->DeleteLocalRef(uPlane);
	env->DeleteLocalRef(vPlane);
	env->DeleteLocalRef(planeArray);
	env->DeleteLocalRef(imageClass);
	env->DeleteLocalRef(planeClass);

	mediaStreamTrack->push(frame);
}
}
} // namespace facebook::react
