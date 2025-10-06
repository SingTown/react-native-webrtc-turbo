package com.webrtc

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack

object Speaker {
    private var audioTrack: AudioTrack? = null
    private val containers = mutableListOf<String>()
    private var playThread: Thread? = null
    private var isPlaying = false

    external fun popAudioStreamTrack(id: String): ByteArray?

    private fun start() {
        if (isPlaying) return
        isPlaying = true
        val bufferSize = AudioTrack.getMinBufferSize(48000, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT)
        audioTrack = AudioTrack(AudioManager.STREAM_MUSIC, 48000, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM)
        audioTrack?.play()
        playThread = Thread {
            while (isPlaying) {
                for (container in containers) {
                    val pcmData = popAudioStreamTrack(container)
                    if (pcmData != null) {
                        audioTrack?.write(pcmData, 0, pcmData.size)
                    }
                }
                Thread.sleep(10)
            }
        }
        playThread?.start()
    }

    private fun stop() {
        if (!isPlaying) return
        isPlaying = false
        audioTrack?.stop()
        audioTrack?.release()
        audioTrack = null
        playThread?.join()
        playThread = null
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
