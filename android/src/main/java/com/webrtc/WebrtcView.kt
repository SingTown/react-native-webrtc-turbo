package com.webrtc

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.Surface
import android.view.TextureView

class WebrtcFabric : TextureView, TextureView.SurfaceTextureListener {
  private var surface: Surface? = null
  private var width: Int = 720
  private var height: Int = 480

  constructor(context: Context) : super(context) {
    init()
  }
  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init()
  }
  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(
    context,
    attrs,
    defStyleAttr
  ) {
    init()
  }

  private fun init() {
    surfaceTextureListener = this
    setLayerType(LAYER_TYPE_HARDWARE, null)
  }

  fun updateFrame(bitmap: Bitmap) {
    try {
      val canvas = lockCanvas() ?: return
      canvas.let { c ->
        c.drawColor(Color.BLACK)
        val srcRect = Rect(0, 0, bitmap.width, bitmap.height)
        val dstRect = Rect(0, 0, this.width, this.height)
        c.drawBitmap(bitmap, srcRect, dstRect, null)
        unlockCanvasAndPost(c)
      }
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
    this.width = width
    this.height = height
    this.surface = Surface(surface)
  }

  override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
    this.width = width
    this.height = height
  }

  override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
    this.surface?.release()
    this.surface = null
    return true
  }

  override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
  }
}
