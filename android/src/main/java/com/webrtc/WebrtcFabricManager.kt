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
  private var videoContainer: String = ""
  private var audioContainer: String = ""

  external fun getFrame(container: String): Bitmap?

  init {
    mDelegate = WebrtcFabricManagerDelegate(this)
    scheduleNextVideoFrame()
  }

  private fun updateVideoFrame() {
    frameExecutor.execute {
      currentView?.let { view ->
        try {
          val bitmap = getFrame(videoContainer)
            ?: return@execute
          frameHandler.post {
            view.updateFrame(bitmap)
            bitmap.recycle()
          }
        } catch (e: Exception) {
          Log.e("WebrtcFabric", "Error updating video frame", e)
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

  private fun scheduleNextVideoFrame() {
    frameHandler.postDelayed({
      updateVideoFrame()
      scheduleNextVideoFrame()
    }, 10L)
  }

  @ReactProp(name = "videoContainer")
  override fun setVideoContainer(view: WebrtcFabric, value: String?) {
    currentView = view
    this.videoContainer = value ?: ""
  }

  @ReactProp(name = "audioContainer")
  override fun setAudioContainer(view: WebrtcFabric, value: String?) {
    val current = value ?: ""
    val old = this.audioContainer
    if (old == current) {
      return
    }

    Sound.removeContainer(old)
    Sound.addContainer(current)
    this.audioContainer = current
  }

  companion object {
    const val NAME = "WebrtcFabric"
  }
}
