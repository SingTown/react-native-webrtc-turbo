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
  override fun createCamera(mediaStreamTrackId: String, promise: Promise) {
    if (ContextCompat.checkSelfPermission(reactApplicationContext, Manifest.permission.CAMERA)
      != PackageManager.PERMISSION_GRANTED
    ) {
      promise.reject("PERMISSION_DENIED", "Camera permission is required")
      return
    }
    Log.e("camera", "start Camera")
    val camera = Camera(reactApplicationContext, mediaStreamTrackId)
    cameras[mediaStreamTrackId] = camera
    Log.e("camera", "end Camera")
    promise.resolve(null)
  }

  @ReactMethod
  override fun requestCameraPermission(promise: Promise) {
    val context = reactApplicationContext
    val currentActivity = context.currentActivity

    if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
      == PackageManager.PERMISSION_GRANTED
    ) {
      promise.resolve(true)
      return
    }

    if (currentActivity is PermissionAwareActivity) {
      val permissionListener = object : com.facebook.react.modules.core.PermissionListener {
        override fun onRequestPermissionsResult(
          requestCode: Int,
          permissions: Array<String>,
          grantResults: IntArray
        ): Boolean {
          if (requestCode == CameraPermissionListener.CAMERA_PERMISSION_REQUEST_CODE) {
            val granted = grantResults.isNotEmpty() &&
              grantResults[0] == PackageManager.PERMISSION_GRANTED
            promise.resolve(granted)
            return true
          }
          return false
        }
      }

      currentActivity.requestPermissions(
        arrayOf(Manifest.permission.CAMERA),
        CameraPermissionListener.CAMERA_PERMISSION_REQUEST_CODE,
        permissionListener
      )
    } else {
      promise.reject("NO_PERMISSION_AWARE_ACTIVITY", "Current activity doesn't support permissions")
    }
  }

  override fun deleteCamera(ms: String) {
    return
  }

  companion object {
    const val NAME = "NativeMediaDevice"
  }
}
