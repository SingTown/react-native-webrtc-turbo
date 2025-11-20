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
  override fun cameraAddPipe(pipeId: String) {
    Camera.addPipe(pipeId)
  }

  override fun cameraRemovePipe(pipeId: String) {
    Camera.removePipe(pipeId)
  }

  override fun microphoneAddPipe(pipeId: String) {
    Microphone.addPipe(pipeId)
  }

  override fun microphoneRemovePipe(pipeId: String) {
    Microphone.removePipe(pipeId)
  }

  companion object {
    const val NAME = "NativeMediaDevice"
  }
}
