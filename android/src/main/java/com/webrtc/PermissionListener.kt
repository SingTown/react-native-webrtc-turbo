package com.webrtc

import android.content.pm.PackageManager
import com.facebook.react.bridge.Promise
import com.facebook.react.modules.core.PermissionListener

class CameraPermissionListener(private val promise: Promise) : PermissionListener {
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ): Boolean {
        if (requestCode == CAMERA_PERMISSION_REQUEST_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                promise.resolve(true)
            } else {
                promise.resolve(false)
            }
            return true
        }
        return false
    }

    companion object {
        const val CAMERA_PERMISSION_REQUEST_CODE = 100
    }
}
