package com.webrtc

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder

object Microphone {
  private var audioRecord: AudioRecord? = null
  private val pipes = mutableListOf<String>()
  private var recordThread: Thread? = null
  private var isRecording = false

  external fun publish(id: String, data: ByteArray, size: Int)

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
        for (pipeId in pipes) {
          publish(pipeId, buffer, read)
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

  fun addPipe(id: String) {
    if (pipes.contains(id)) {
      return
    }
    pipes.add(id)
    if (pipes.size == 1) {
      start()
    }
  }

  fun removePipe(id: String) {
    if (!pipes.contains(id)) {
      return
    }
    pipes.remove(id)
    if (pipes.isEmpty()) {
      stop()
    }
  }
}
