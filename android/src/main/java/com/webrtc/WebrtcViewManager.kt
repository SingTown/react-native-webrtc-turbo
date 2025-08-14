package com.webrtc

import android.graphics.Color
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.uimanager.annotations.ReactProp
import com.facebook.react.viewmanagers.WebrtcViewManagerInterface
import com.facebook.react.viewmanagers.WebrtcViewManagerDelegate

@ReactModule(name = WebrtcViewManager.NAME)
class WebrtcViewManager : SimpleViewManager<WebrtcView>(),
  WebrtcViewManagerInterface<WebrtcView> {
  private val mDelegate: ViewManagerDelegate<WebrtcView>

  init {
    mDelegate = WebrtcViewManagerDelegate(this)
  }

  override fun getDelegate(): ViewManagerDelegate<WebrtcView>? {
    return mDelegate
  }

  override fun getName(): String {
    return NAME
  }

  public override fun createViewInstance(context: ThemedReactContext): WebrtcView {
    return WebrtcView(context)
  }

  @ReactProp(name = "color")
  override fun setColor(view: WebrtcView?, color: String?) {
    view?.setBackgroundColor(Color.parseColor(color))
  }

  companion object {
    const val NAME = "WebrtcView"
  }
}
