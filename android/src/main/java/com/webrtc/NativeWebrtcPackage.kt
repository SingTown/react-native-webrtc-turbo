package com.webrtc

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider
import com.facebook.react.uimanager.ViewManager

class NativeMediaDevicePackage : BaseReactPackage() {

  override fun createViewManagers(reactContext: ReactApplicationContext): List<ViewManager<*, *>> {
    return listOf(WebrtcFabricManager(reactContext))
  }

  override fun getModule(name: String, reactContext: ReactApplicationContext): NativeModule? =
    when (name) {
      NativeMediaDeviceModule.NAME -> NativeMediaDeviceModule(reactContext)
      WebrtcFabricManager.NAME -> WebrtcFabricManager(reactContext)
      else -> null
    }

  override fun getReactModuleInfoProvider() = ReactModuleInfoProvider {
    mapOf(
      NativeMediaDeviceModule.NAME to ReactModuleInfo(
        name = NativeMediaDeviceModule.NAME,
        className = NativeMediaDeviceModule.NAME,
        canOverrideExistingModule = false,
        needsEagerInit = false,
        isCxxModule = false,
        isTurboModule = true
      ),
      WebrtcFabricManager.NAME to ReactModuleInfo(
        name = WebrtcFabricManager.NAME,
        className = WebrtcFabricManager.NAME,
        canOverrideExistingModule = false,
        needsEagerInit = false,
        isCxxModule = false,
        isTurboModule = true,
      )
    )
  }
}