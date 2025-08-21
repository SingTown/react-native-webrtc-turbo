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
import com.facebook.react.viewmanagers.WebrtcViewManagerInterface
import com.facebook.react.viewmanagers.WebrtcViewManagerDelegate

class DataChannelLib {
    external fun getTrackBuffer(): ByteBuffer
}

@ReactModule(name = WebrtcViewManager.NAME)
class WebrtcViewManager : SimpleViewManager<WebrtcView>(),
  WebrtcViewManagerInterface<WebrtcView> {
  private val mDelegate: ViewManagerDelegate<WebrtcView>
  private val dataChannelLib: DataChannelLib
  private val frameHandler = Handler(Looper.getMainLooper())
  private var currentView: WebrtcView? = null

  init {
    mDelegate = WebrtcViewManagerDelegate(this)
    dataChannelLib = DataChannelLib()
    scheduleNextFrame()
  }

  private fun updateFrame() {
    currentView?.let { view ->
      try {
        val bufferData = dataChannelLib.getTrackBuffer()

        bufferData.rewind()
        val bitmap = createBitmap(720, 480, Bitmap.Config.ARGB_8888)
        bitmap.copyPixelsFromBuffer(bufferData)

        view.updateFrame(bitmap)
        bitmap.recycle()

      } catch (e: Exception) {
        Log.e("WebrtcView", "Error updating frame", e)
      }
    }
  }

  override fun getDelegate(): ViewManagerDelegate<WebrtcView> {
    return mDelegate
  }

  override fun getName(): String {
    return NAME
  }

  public override fun createViewInstance(context: ThemedReactContext): WebrtcView {
    return WebrtcView(context)
  }

  private fun scheduleNextFrame() {
    frameHandler.postDelayed({
      updateFrame()
      scheduleNextFrame()
    }, 33L) // 33ms = ~30fps
  }

  @ReactProp(name = "trackId")
  override fun setTrackId(view: WebrtcView?, trackId: Int) {
    currentView = view
  }

  companion object {
    const val NAME = "WebrtcView"
  }
}
