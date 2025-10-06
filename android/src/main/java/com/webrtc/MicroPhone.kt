package com.webrtc

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder

object MicroPhone {
    private var audioRecord: AudioRecord? = null
    private val containers = mutableListOf<String>()
    private var recordThread: Thread? = null
    private var isRecording = false

    external fun pushAudioStreamTrack(id: String, data: ByteArray, size: Int)

    private fun start() {
        if (isRecording) return
        isRecording = true
        val bufferSize = AudioRecord.getMinBufferSize(48000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT)
        audioRecord = AudioRecord(MediaRecorder.AudioSource.MIC, 48000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferSize)
        audioRecord?.startRecording()
        recordThread = Thread {
            val buffer = ByteArray(bufferSize)
            while (isRecording) {
                val read = audioRecord?.read(buffer, 0, buffer.size) ?: 0
                if (read < 0) {
                    continue
                }
                for (container in containers) {
                    pushAudioStreamTrack(container, buffer, read)
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

    fun push(container: String) {
        if (containers.contains(container)) {
            return
        }
        containers.add(container)
        if (containers.size == 1) {
            start()
        }
    }

    fun pop(container: String) {
        if (!containers.contains(container)) {
            return
        }
        containers.remove(container)
        if (containers.isEmpty()) {
            stop()
        }
    }
}
