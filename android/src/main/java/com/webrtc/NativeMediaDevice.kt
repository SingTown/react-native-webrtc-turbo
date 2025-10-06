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
  override fun cameraAddContainer(container: String) {
    Camera.addContainer(container)
  }

  override fun cameraRemoveContainer(container: String) {
    Camera.removeContainer(container)
  }

  override fun microphoneAddContainer(container: String) {
    Microphone.addContainer(container)
  }

  override fun microphoneRemoveContainer(container: String) {
    Microphone.removeContainer(container)
  }

  companion object {
    const val NAME = "NativeMediaDevice"
  }
}
