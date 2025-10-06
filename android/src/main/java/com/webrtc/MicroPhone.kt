package com.webrtc

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder

object Microphone {
  private var audioRecord: AudioRecord? = null
  private val containers = mutableListOf<String>()
  private var recordThread: Thread? = null
  private var isRecording = false

  external fun processFrame(id: String, data: ByteArray, size: Int)

  private fun start() {
    if (isRecording) return
    isRecording = true
    val bufferSize = AudioRecord.getMinBufferSize(
      48000,
      AudioFormat.CHANNEL_IN_MONO,
      AudioFormat.ENCODING_PCM_16BIT
    )
    audioRecord = AudioRecord(
      MediaRecorder.AudioSource.MIC,
      48000,
      AudioFormat.CHANNEL_IN_MONO,
      AudioFormat.ENCODING_PCM_16BIT,
      bufferSize
    )
    audioRecord?.startRecording()
    recordThread = Thread {
      val buffer = ByteArray(bufferSize)
      while (isRecording) {
        val read = audioRecord?.read(buffer, 0, buffer.size) ?: 0
        if (read < 0) {
          continue
        }
        for (container in containers) {
          processFrame(container, buffer, read)
        }
      }
    }
    recordThread?.start()
  }

  private fun stop() {
    if (!isRecording) return
    isRecording = false
    audioRecord?.stop()
    audioRecord?.release()
    audioRecord = null
    recordThread = null
  }

  fun addContainer(id: String) {
    if (containers.contains(id)) {
      return
    }
    containers.add(id)
    if (containers.size == 1) {
      start()
    }
  }

  fun removeContainer(id: String) {
    if (!containers.contains(id)) {
      return
    }
    containers.remove(id)
    if (containers.isEmpty()) {
      stop()
    }
  }
}
