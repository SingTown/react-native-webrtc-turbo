package com.webrtc

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.hardware.camera2.*
import android.media.Image
import android.media.ImageReader
import android.os.Handler
import android.os.HandlerThread
import androidx.core.content.ContextCompat
import android.util.Log

class Camera(
  private val context: Context,
  private val mediaStreamTrackId: String,
) {

  external fun pushVideoStreamTrack(mediaStreamTrackId: String, image: Image)

  companion object {
    private const val DEFAULT_WIDTH = 1280
    private const val DEFAULT_HEIGHT = 720
  }

  private var cameraDevice: CameraDevice? = null
  private var captureSession: CameraCaptureSession? = null
  private var imageReader: ImageReader? = null
  private var backgroundHandler: Handler? = null
  private var backgroundThread: HandlerThread? = null

  val sessionStateCallback = object : CameraCaptureSession.StateCallback() {
    override fun onConfigured(session: CameraCaptureSession) {
      captureSession = session
      start()
    }

    override fun onConfigureFailed(session: CameraCaptureSession) {
      throw RuntimeException("Camera configuration failed")
    }
  }

  private val cameraStateCallback = object : CameraDevice.StateCallback() {
    override fun onOpened(camera: CameraDevice) {
      cameraDevice = camera
      cameraDevice?.createCaptureSession(
        listOf(imageReader!!.surface),
        sessionStateCallback,
        backgroundHandler
      )
    }

    override fun onDisconnected(camera: CameraDevice) {
      cameraDevice = null
      camera.close()
    }

    override fun onError(camera: CameraDevice, error: Int) {
      cameraDevice = null
      throw RuntimeException("Camera run Error!")
    }
  }

  init {
    val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    backgroundThread = HandlerThread("CameraBackground").apply { start() }
    backgroundHandler = Handler(backgroundThread!!.looper)
    if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) !=
      PackageManager.PERMISSION_GRANTED
    ) {
      throw RuntimeException("Camera Permission Error!")
    }

    val cameraId = cameraManager.cameraIdList.firstOrNull()
    Log.e("webrtc", "cameraId:" + cameraId.toString())
    if (cameraId == null)
      throw RuntimeException("Camera not exist!")

    imageReader = ImageReader.newInstance(DEFAULT_WIDTH, DEFAULT_HEIGHT, ImageFormat.YUV_420_888, 2)
    val onImageAvailableListener = ImageReader.OnImageAvailableListener { reader ->
      val image = reader.acquireNextImage()
      image?.use { pushVideoStreamTrack(mediaStreamTrackId, it) }
    }
    imageReader?.setOnImageAvailableListener(onImageAvailableListener, backgroundHandler)
    cameraManager.openCamera(cameraId, cameraStateCallback, backgroundHandler)
  }

  fun dispose() {
    captureSession?.stopRepeating()

    captureSession?.close()
    captureSession = null

    cameraDevice?.close()
    cameraDevice = null

    imageReader?.close()
    imageReader = null

    backgroundHandler?.post {
      backgroundThread?.quitSafely()
    }

    backgroundThread = null
    backgroundHandler = null
  }

  fun start() {
    if (cameraDevice != null && captureSession != null) {
      val captureRequestBuilder = cameraDevice?.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
      captureRequestBuilder?.addTarget(imageReader?.surface!!)

      captureRequestBuilder?.set(
        CaptureRequest.CONTROL_AF_MODE,
        CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
      )

      captureRequestBuilder?.set(
        CaptureRequest.CONTROL_AE_MODE,
        CaptureRequest.CONTROL_AE_MODE_ON
      )

      captureRequestBuilder?.set(
        CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE,
        android.util.Range(30, 30)
      )


      val captureRequest = captureRequestBuilder?.build()
      captureSession?.setRepeatingRequest(captureRequest!!, null, backgroundHandler)
    }
  }
}
