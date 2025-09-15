package com.webrtc

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack

object Speaker {
    private var audioTrack: AudioTrack? = null
    private val trackIds = mutableListOf<String>()
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
                for (id in trackIds) {
                    val pcmData = popAudioStreamTrack(id)
                    if (pcmData != null) {
                        audioTrack?.write(pcmData, 0, pcmData.size)
                    }
                }
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
        playThread = null
    }

    fun pushMediaStreamTrackId(id: String) {
        if (!trackIds.contains(id)) {
            trackIds.add(id)
        }
        if (trackIds.size == 1) {
            start()
        }
    }

    fun popMediaStreamTrackId(id: String) {
        trackIds.remove(id)
        if (trackIds.isEmpty()) {
            stop()
        }
    }
}
