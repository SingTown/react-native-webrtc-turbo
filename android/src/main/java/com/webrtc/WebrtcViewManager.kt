package com.webrtc

import java.nio.ByteBuffer
import android.graphics.*
import android.util.Log
import android.os.Handler
import android.os.Looper
import androidx.core.graphics.createBitmap
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.uimanager.annotations.ReactProp
import com.facebook.react.viewmanagers.WebrtcFabricManagerInterface
import com.facebook.react.viewmanagers.WebrtcFabricManagerDelegate

class RGBAFrame(
  val width: Int,
  val height: Int,
  val lineSize: Int,
  val buffer: ByteArray,
)

@ReactModule(name = WebrtcFabricManager.NAME)
class WebrtcFabricManager : SimpleViewManager<WebrtcFabric>(),
  WebrtcFabricManagerInterface<WebrtcFabric> {
  private val mDelegate: ViewManagerDelegate<WebrtcFabric>

  private val frameHandler = Handler(Looper.getMainLooper())
  private var currentView: WebrtcFabric? = null
  private var videoTrackId: String = ""
  private var audioTrackId: String = ""

  external fun getTrackBuffer(trackId: String): RGBAFrame?

  init {
    mDelegate = WebrtcFabricManagerDelegate(this)
    scheduleNextFrame()
  }

  private fun updateFrame() {
    currentView?.let { view ->
      try {
        val rgbaFrame = getTrackBuffer(videoTrackId)
          ?: return
        val bitmap = createBitmap(rgbaFrame.width, rgbaFrame.height, Bitmap.Config.ARGB_8888)
        bitmap.copyPixelsFromBuffer(ByteBuffer.wrap(rgbaFrame.buffer))

        view.updateFrame(bitmap)
        bitmap.recycle()

      } catch (e: Exception) {
        Log.e("WebrtcFabric", "Error updating frame", e)
      }
    }
  }

  override fun getDelegate(): ViewManagerDelegate<WebrtcFabric> {
    return mDelegate
  }

  override fun getName(): String {
    return NAME
  }

  public override fun createViewInstance(context: ThemedReactContext): WebrtcFabric {
    return WebrtcFabric(context)
  }

  private fun scheduleNextFrame() {
    frameHandler.postDelayed({
      updateFrame()
      scheduleNextFrame()
    }, 10L)
  }

  @ReactProp(name = "videoTrackId")
  override fun setVideoTrackId(view: WebrtcFabric, value: String?) {
    currentView = view
    this.videoTrackId = value ?: ""
  }

  @ReactProp(name = "audioTrackId")
  override fun setAudioTrackId(view: WebrtcFabric, value: String?) {
    this.audioTrackId = value ?: ""
  }

  companion object {
    const val NAME = "WebrtcFabric"
  }
}
