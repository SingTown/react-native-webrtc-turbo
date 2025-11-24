package com.webrtc

import android.graphics.*
import android.util.Log
import android.os.Handler
import android.os.Looper
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.view.Surface
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

  private var currentView: WebrtcFabric? = null
  private var videoSubscriptionId: Int = -1
  private var audioSubscriptionId: Int = -1
  private var videoPipeId: String = ""
  private var audioPipeId: String = ""
  private var resizeMode: String = "contain"
  private var audioTrack = AudioTrack(
    AudioManager.STREAM_MUSIC,
    48000,
    AudioFormat.CHANNEL_OUT_STEREO,
    AudioFormat.ENCODING_PCM_16BIT,
    AudioTrack.getMinBufferSize(
      48000,
      AudioFormat.CHANNEL_OUT_STEREO,
      AudioFormat.ENCODING_PCM_16BIT
    ),
    AudioTrack.MODE_STREAM
  )

  external fun subscribeVideo(pipeId: String, surface: Surface?): Int
  external fun subscribeAudio(pipeId: String): Int
  external fun unsubscribe(subscriptionId: Int)

  init {
    mDelegate = WebrtcFabricManagerDelegate(this)
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

  private fun applyVideoPipeId(surface: Surface?, newVideoPipeId: String) {
    if (surface == null) {
      return;
    }
    if (newVideoPipeId.isEmpty()) {
      return;
    }
    if (this.videoPipeId == newVideoPipeId) {
      return
    }
    if (this.videoSubscriptionId > 0) {
      this.unsubscribe(this.videoSubscriptionId)
    }
    this.videoSubscriptionId = subscribeVideo(newVideoPipeId, surface)
    this.videoPipeId = newVideoPipeId
  }

  @ReactProp(name = "videoPipeId")
  override fun setVideoPipeId(view: WebrtcFabric, value: String?) {
    currentView = view
    applyVideoPipeId(view.getSurface(), value ?: "")
    view.setOnSurfaceAvailableListener { surface ->
      applyVideoPipeId(surface, value ?: "")
    }
  }

  @ReactProp(name = "audioPipeId")
  override fun setAudioPipeId(view: WebrtcFabric, value: String?) {
    val current = value ?: ""
    val old = this.audioPipeId
    if (old == current) {
      return
    }

    audioTrack.play()
    if (this.audioSubscriptionId > 0) {
      this.unsubscribe(this.audioSubscriptionId)
    }

    this.audioSubscriptionId = subscribeAudio(current)
    this.audioPipeId = current
  }

  @ReactProp(name = "resizeMode")
  override fun setResizeMode(view: WebrtcFabric, value: String?) {
    val current = value ?: "contain"
    val old = this.resizeMode
    if (old == current) {
      return
    }
  }

  companion object {
    const val NAME = "WebrtcFabric"
  }
}
