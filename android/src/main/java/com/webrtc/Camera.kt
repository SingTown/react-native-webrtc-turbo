package com.webrtc

import android.Manifest
import android.content.Context
import android.graphics.ImageFormat
import android.hardware.camera2.*
import android.media.Image
import android.media.ImageReader
import android.os.Handler
import android.os.HandlerThread
import androidx.annotation.RequiresPermission

object Camera {
  external fun publish(mediaStreamTrackId: String, image: Image)

  private const val DEFAULT_WIDTH = 1280
  private const val DEFAULT_HEIGHT = 720

  private var cameraManager: CameraManager? = null
  private var cameraDevice: CameraDevice? = null
  private var captureSession: CameraCaptureSession? = null
  private var backgroundThread = HandlerThread("CameraBackground")
  private var imageReader =
    ImageReader.newInstance(DEFAULT_WIDTH, DEFAULT_HEIGHT, ImageFormat.YUV_420_888, 2)
  private val containers = mutableListOf<String>()

  fun init(context: Context) {
    cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
  }

  private val cameraStateCallback = object : CameraDevice.StateCallback() {
    override fun onOpened(camera: CameraDevice) {
      cameraDevice = camera
      val captureRequestBuilder = camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
      captureRequestBuilder.addTarget(imageReader.surface)

      captureRequestBuilder.set(
        CaptureRequest.CONTROL_AF_MODE,
        CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
      )

      captureRequestBuilder.set(
        CaptureRequest.CONTROL_AE_MODE,
        CaptureRequest.CONTROL_AE_MODE_ON
      )

      captureRequestBuilder.set(
        CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE,
        android.util.Range(30, 30)
      )
      val captureRequest = captureRequestBuilder.build()

      val sessionStateCallback = object : CameraCaptureSession.StateCallback() {
        override fun onConfigured(session: CameraCaptureSession) {
          captureSession = session
          session.setRepeatingRequest(captureRequest, null, Handler(backgroundThread.looper))
        }

        override fun onConfigureFailed(session: CameraCaptureSession) {
          throw RuntimeException("Camera configuration failed")
        }
      }
      camera.createCaptureSession(
        listOf(imageReader.surface),
        sessionStateCallback,
        Handler(backgroundThread.looper)
      )
    }

    override fun onDisconnected(camera: CameraDevice) {
      camera.close()
    }

    override fun onError(camera: CameraDevice, error: Int) {
      camera.close()
      throw RuntimeException("Camera run Error!")
    }
  }

  @RequiresPermission(Manifest.permission.CAMERA)
  fun start() {
    backgroundThread = HandlerThread("CameraBackground")
    backgroundThread.start()
    val onImageAvailableListener = ImageReader.OnImageAvailableListener { reader ->
      val image = reader.acquireNextImage()
      image.use {
        for (container in containers) {
          publish(container, it)
        }
      }
    }
    imageReader.setOnImageAvailableListener(
      onImageAvailableListener,
      Handler(backgroundThread.looper)
    )
    val cameraId = cameraManager?.cameraIdList?.firstOrNull()
    if (cameraId == null)
      throw RuntimeException("Camera not exist!")
    cameraManager?.openCamera(cameraId, cameraStateCallback, Handler(backgroundThread.looper))
  }

  fun stop() {
    captureSession?.stopRepeating()
    captureSession?.close()
    captureSession = null
    cameraDevice?.close()
    cameraDevice = null
    backgroundThread.quitSafely()
    backgroundThread.join()
  }

  @RequiresPermission(Manifest.permission.CAMERA)
  fun addPipe(id: String) {
    if (containers.contains(id)) {
      return
    }
    containers.add(id)
    if (containers.size == 1) {
      start()
    }
  }

  fun removePipe(id: String) {
    if (!containers.contains(id)) {
      return
    }
    containers.remove(id)
    if (containers.isEmpty()) {
      stop()
    }
  }
}
