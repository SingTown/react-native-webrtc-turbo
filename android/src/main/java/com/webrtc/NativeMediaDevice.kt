package com.webrtc

import android.Manifest
import com.facebook.react.bridge.ReactApplicationContext
import androidx.core.content.ContextCompat
import android.content.pm.PackageManager
import android.util.Log
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.modules.core.PermissionAwareActivity

class NativeMediaDeviceModule(reactContext: ReactApplicationContext) :
  NativeMediaDeviceSpec(reactContext) {

  private val cameras = mutableMapOf<String, Camera>()

  override fun getName() = NAME

  @ReactMethod
  override fun requestPermission(name:String, promise: Promise) {
    requestPermission(reactApplicationContext, name, promise)
  }

  @ReactMethod
  override fun createCamera(mediaStreamTrackId: String, promise: Promise) {
    if (ContextCompat.checkSelfPermission(reactApplicationContext, Manifest.permission.CAMERA)
      != PackageManager.PERMISSION_GRANTED
    ) {
      promise.reject("PERMISSION_DENIED", "Camera permission is required")
      return
    }
    val camera = Camera(reactApplicationContext, mediaStreamTrackId)
    cameras[mediaStreamTrackId] = camera
    promise.resolve(null)
  }

  override fun deleteCamera(mediaStreamTrackId: String) {
    if (cameras.containsKey(mediaStreamTrackId)) {
      cameras[mediaStreamTrackId]?.dispose()
      cameras.remove(mediaStreamTrackId)
    }
  }

  override fun createAudio(mediaStreamTrackId: String, promise: Promise) {
    if (ContextCompat.checkSelfPermission(reactApplicationContext, Manifest.permission.RECORD_AUDIO)
      != PackageManager.PERMISSION_GRANTED
    ) {
      promise.reject("PERMISSION_DENIED", "Microphone permission is required")
      return
    }
    MicroPhone.pushMediaStreamTrackId(mediaStreamTrackId)
    promise.resolve(null)
  }

  override fun deleteAudio(mediaStreamTrackId: String) {
    MicroPhone.popMediaStreamTrackId(mediaStreamTrackId)
  }

  companion object {
    const val NAME = "NativeMediaDevice"
  }
}
