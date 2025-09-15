
#pragma once

#include "ffmpeg.h"
#include "guid.h"
#include "log.h"
#include <functional>
#include <mutex>
#include <queue>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAME_SIZE 960

class MediaStreamTrack {
  protected:
	std::recursive_mutex mutex;

  public:
	MediaStreamTrack() = default;
	virtual std::string type() = 0;
	virtual void push(std::shared_ptr<AVFrame> frame) = 0;
	virtual std::shared_ptr<AVFrame> pop() = 0;
	std::function<void(std::shared_ptr<AVFrame>)> onPushCallback;

	void onPush(std::function<void(std::shared_ptr<AVFrame>)> callback) {
		std::lock_guard lock(mutex);
		onPushCallback = callback;
	}
	virtual ~MediaStreamTrack() { onPushCallback = nullptr; }
};

struct ComparePTS {
	bool operator()(std::shared_ptr<AVFrame> &a,
	                std::shared_ptr<AVFrame> &b) const {
		return a->pts > b->pts;
	}
};

class VideoStreamTrack : public MediaStreamTrack {
  private:
	std::priority_queue<std::shared_ptr<AVFrame>,
	                    std::vector<std::shared_ptr<AVFrame>>, ComparePTS>
	    queue;
	Scaler scalerIn;
	Scaler scalerOut;

  public:
	std::string type() override { return "video"; }
	void push(std::shared_ptr<AVFrame> frame) override {
		std::function<void(std::shared_ptr<AVFrame>)> callbackCopy;
		{
			std::lock_guard lock(mutex);
			if (!frame) {
				return;
			}
			if (queue.size() > 10) {
				queue.pop(); // drop
			}
			queue.push(frame);
			callbackCopy = onPushCallback;
		}

		if (callbackCopy) {
			callbackCopy(frame);
		}
	}

	void pushVideo(std::shared_ptr<AVFrame> frame, AVPixelFormat format,
	               int width, int height) {
		push(scalerIn.scale(frame, format, width, height));
	}

	std::shared_ptr<AVFrame> pop() override {
		std::lock_guard lock(mutex);
		if (queue.empty()) {
			return nullptr;
		}
		auto frame = queue.top();
		queue.pop();
		return frame;
	}

	std::shared_ptr<AVFrame> popVideo(AVPixelFormat format) {
		auto frame = pop();
		if (!frame) {
			return nullptr;
		}
		return scalerOut.scale(frame, format, frame->width, frame->height);
	}
};

class AudioStreamTrack : public MediaStreamTrack {
  private:
	int64_t pts;
	std::vector<float> pcm;
	Resampler resamplerIn;
	Resampler resamplerOut;

  public:
	std::string type() override { return "audio"; }

	void push(std::shared_ptr<AVFrame> frame) override {
		std::function<void(std::shared_ptr<AVFrame>)> callbackCopy;
		{
			std::lock_guard lock(mutex);
			if (!frame) {
				return;
			}
			if (pcm.empty()) {
				pts = frame->pts;
			}
			if (pcm.size() > 960 * 10) {
				// drop
				pcm.erase(pcm.begin(), pcm.begin() + frame->nb_samples);
			}

			if (frame->format != AV_SAMPLE_FMT_FLT ||
			    frame->sample_rate != SAMPLE_RATE ||
			    frame->ch_layout.nb_channels != CHANNELS) {
				frame = resamplerIn.resample(frame, AV_SAMPLE_FMT_FLT,
				                             SAMPLE_RATE, CHANNELS);
			}
			auto data = (float *)frame->data[0];
			pcm.insert(pcm.end(), data,
			           data + frame->nb_samples * frame->ch_layout.nb_channels);
			callbackCopy = onPushCallback;
		}

		if (callbackCopy) {
			callbackCopy(frame);
		}
	}

	std::shared_ptr<AVFrame> pop() override {
		return popAudio(AV_SAMPLE_FMT_FLT, 48000, 2);
	}

	std::shared_ptr<AVFrame> popAudio(AVSampleFormat format, int sampleRate,
	                                  int channels) {
		std::lock_guard lock(mutex);
		if (pcm.size() < FRAME_SIZE * CHANNELS) {
			return nullptr;
		}
		auto frame = createAudioFrame(AV_SAMPLE_FMT_FLT, pts, SAMPLE_RATE,
		                              CHANNELS, FRAME_SIZE);
		pts += frame->nb_samples;
		memcpy(frame->data[0], pcm.data(),
		       frame->nb_samples * sizeof(float) * CHANNELS);
		pcm.erase(pcm.begin(), pcm.begin() + frame->nb_samples * CHANNELS);
		if (format != AV_SAMPLE_FMT_FLT || sampleRate != SAMPLE_RATE ||
		    channels != CHANNELS) {
			return resamplerOut.resample(frame, format, sampleRate, channels);
		} else {
			return frame;
		}
	}
};

std::shared_ptr<MediaStreamTrack> getMediaStreamTrack(const std::string &id);
void eraseMediaStreamTrack(const std::string &id);

std::shared_ptr<VideoStreamTrack> getVideoStreamTrack(const std::string &id);
std::string emplaceVideoStreamTrack(std::shared_ptr<VideoStreamTrack> ptr);
void eraseVideoStreamTrack(const std::string &id);

std::shared_ptr<AudioStreamTrack> getAudioStreamTrack(const std::string &id);
std::string emplaceAudioStreamTrack(std::shared_ptr<AudioStreamTrack> ptr);
void eraseAudioStreamTrack(const std::string &id);
