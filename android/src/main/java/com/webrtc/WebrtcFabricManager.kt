package com.webrtc

import android.graphics.*
import android.util.Log
import android.os.Handler
import android.os.Looper
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.uimanager.annotations.ReactProp
import com.facebook.react.viewmanagers.WebrtcFabricManagerInterface
import com.facebook.react.viewmanagers.WebrtcFabricManagerDelegate
import java.util.concurrent.Executors
import java.util.concurrent.ExecutorService

@ReactModule(name = WebrtcFabricManager.NAME)
class WebrtcFabricManager(context: ReactApplicationContext) : SimpleViewManager<WebrtcFabric>(),
  WebrtcFabricManagerInterface<WebrtcFabric> {
  private val mDelegate: ViewManagerDelegate<WebrtcFabric>

  private val frameHandler = Handler(Looper.getMainLooper())
  private val frameExecutor: ExecutorService = Executors.newCachedThreadPool()
  private var currentView: WebrtcFabric? = null
  private var videoStreamTrackId: String = ""
  private var audioStreamTrackId: String = ""

  external fun popVideoStreamTrack(id: String): Bitmap?

  init {
    mDelegate = WebrtcFabricManagerDelegate(this)
    scheduleNextFrame()
  }

  private fun updateFrame() {
    frameExecutor.execute {
      currentView?.let { view ->
        try {
          val bitmap = popVideoStreamTrack(videoStreamTrackId)
            ?: return@execute
          frameHandler.post {
            view.updateFrame(bitmap)
            bitmap.recycle()
          }
        } catch (e: Exception) {
          Log.e("WebrtcFabric", "Error updating frame", e)
        }
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

  @ReactProp(name = "videoStreamTrackId")
  override fun setVideoStreamTrackId(view: WebrtcFabric, value: String?) {
    currentView = view
    this.videoStreamTrackId = value ?: ""
  }

  @ReactProp(name = "audioStreamTrackId")
  override fun setAudioStreamTrackId(view: WebrtcFabric, value: String?) {
    this.audioStreamTrackId = value ?: ""
  }

  companion object {
    const val NAME = "WebrtcFabric"
  }
}
