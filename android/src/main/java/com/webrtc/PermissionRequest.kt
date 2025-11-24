package com.webrtc

import android.content.pm.PackageManager
import com.facebook.react.bridge.Promise
import com.facebook.react.modules.core.PermissionListener
import com.facebook.react.bridge.ReactApplicationContext
import androidx.core.content.ContextCompat
import com.facebook.react.modules.core.PermissionAwareActivity
import kotlin.random.Random

fun requestPermission(context: ReactApplicationContext, name: String, promise: Promise) {
  val currentActivity = context.currentActivity
  val permission: String
  if (name == "camera") {
    permission = android.Manifest.permission.CAMERA
  } else if (name == "microphone") {
    permission = android.Manifest.permission.RECORD_AUDIO
  } else {
    promise.reject("UNKNOWN_PERMISSION", "Unknown permission: $name")
    return
  }

  if (ContextCompat.checkSelfPermission(context, permission)
    == PackageManager.PERMISSION_GRANTED
  ) {
    promise.resolve(true)
    return
  }

  val code = Random.nextInt(0, 65535)
  if (currentActivity is PermissionAwareActivity) {
    val permissionListener = object : PermissionListener {
      override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
      ): Boolean {
        if (requestCode == code) {
          if (permissions.isEmpty() || grantResults.isEmpty()) {
            promise.reject(
              "NotAllowedError",
              "Permission denied by user"
            )
            return true
          }

          if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            promise.resolve(true)
          } else {
            promise.reject(
              "NotAllowedError",
              "Permission denied by user"
            )
          }
          return true
        }
        return true
      }
    }

    currentActivity.requestPermissions(
      arrayOf(permission),
      code,
      permissionListener
    )
  } else {
    promise.reject("NO_PERMISSION_AWARE_ACTIVITY", "Current activity doesn't support permissions")
  }
}
