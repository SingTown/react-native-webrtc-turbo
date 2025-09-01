package com.webrtc

import android.Manifest
import android.content.Context.CAMERA_SERVICE
import com.facebook.react.bridge.ReactApplicationContext
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureRequest
import android.os.Handler
import android.os.Looper
import android.os.HandlerThread
import androidx.annotation.RequiresPermission
import androidx.core.content.ContextCompat
import androidx.core.app.ActivityCompat
import android.content.pm.PackageManager
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.modules.core.PermissionAwareActivity

class NativeMediaDeviceModule(reactContext: ReactApplicationContext) :
  NativeMediaDeviceSpec(reactContext) {

  override fun getName() = NAME

  @ReactMethod
  override fun createCamera(ms: String, promise: Promise) {
    if (ContextCompat.checkSelfPermission(reactApplicationContext, Manifest.permission.CAMERA)
        != PackageManager.PERMISSION_GRANTED) {
      promise.reject("PERMISSION_DENIED", "Camera permission is required")
      return
    }

    try {
      val cameraManager = reactApplicationContext.getSystemService(CAMERA_SERVICE) as CameraManager
      val cameraId = cameraManager.cameraIdList[0]
      val handler = Handler(Looper.getMainLooper())

      cameraManager.openCamera(cameraId, object : CameraDevice.StateCallback() {
          override fun onOpened(camera: CameraDevice) {
              promise.resolve("Camera opened successfully")
          }

          override fun onDisconnected(camera: CameraDevice) {
              camera.close()
              promise.reject("CAMERA_DISCONNECTED", "Camera was disconnected")
          }

          override fun onError(camera: CameraDevice, error: Int) {
              camera.close()
              promise.reject("CAMERA_ERROR", "Camera error: $error")
          }
      }, handler)
    } catch (e: Exception) {
      promise.reject("CAMERA_OPEN_FAILED", e.message, e)
    }
  }

  @ReactMethod
  override fun requestCameraPermission(promise: Promise) {
    val context = reactApplicationContext
    val currentActivity = context.currentActivity
    
    if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
        == PackageManager.PERMISSION_GRANTED) {
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
