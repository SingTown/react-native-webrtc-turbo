package com.webrtc

import android.Manifest
import com.facebook.react.bridge.ReactApplicationContext
import androidx.annotation.RequiresPermission
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactMethod

class NativeMediaDeviceModule(reactContext: ReactApplicationContext) :
  NativeMediaDeviceSpec(reactContext) {

  override fun getName() = NAME

  init {
    Camera.init(reactApplicationContext)
  }

  @ReactMethod
  override fun requestPermission(name: String, promise: Promise) {
    requestPermission(reactApplicationContext, name, promise)
  }

  @RequiresPermission(Manifest.permission.CAMERA)
  @ReactMethod
  override fun cameraPush(mediaStreamTrackId: String) {
    Camera.push(mediaStreamTrackId)
  }

  override fun cameraPop(mediaStreamTrackId: String) {
    Camera.pop(mediaStreamTrackId)
  }

  override fun microphonePush(mediaStreamTrackId: String) {
    MicroPhone.push(mediaStreamTrackId)
  }

  override fun microphonePop(mediaStreamTrackId: String) {
    MicroPhone.pop(mediaStreamTrackId)
  }

  companion object {
    const val NAME = "NativeMediaDevice"
  }
}
