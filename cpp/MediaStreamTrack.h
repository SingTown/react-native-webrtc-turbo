
#pragma once

#include "ffmpeg.h"
#include "guid.h"
#include "log.h"
#include <functional>
#include <mutex>
#include <queue>

class MediaStreamTrack {
  private:
	std::queue<std::shared_ptr<AVFrame>> queue;
	std::mutex mutex;
	std::function<void(std::shared_ptr<AVFrame>)> onPushCallback;
	struct SwsContext *sws_ctx;

  public:
	~MediaStreamTrack() {
		if (sws_ctx) {
			sws_freeContext(sws_ctx);
			sws_ctx = nullptr;
		}
	}

	void push(std::shared_ptr<AVFrame> frame) {
		std::function<void(std::shared_ptr<AVFrame>)> callbackCopy;
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (!frame) {
				return;
			}
			if (queue.size() > 100) {
				queue.pop(); // drop
			}
			queue.push(frame);
			callbackCopy = onPushCallback;
		}

		if (callbackCopy) {
			callbackCopy(frame);
		}
	}

	std::shared_ptr<AVFrame> pop(AVPixelFormat format) {
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.empty()) {
			return nullptr;
		}
		auto frame = queue.front();
		queue.pop();

		if (frame->format == format) {
			return frame;
		}

		auto dst = createAVFrame();
		if (!dst)
			throw std::runtime_error("Could not allocate AVFrame");
		dst->format = format;
		dst->width = frame->width;
		dst->height = frame->height;
		dst->pts = frame->pts;
		if (av_frame_get_buffer(dst.get(), 32) < 0) {
			throw std::runtime_error("Could not allocate RGBA image");
		}

		sws_ctx = sws_getCachedContext(sws_ctx, frame->width, frame->height,
		                               (AVPixelFormat)frame->format, dst->width,
		                               dst->height, (AVPixelFormat)dst->format,
		                               SWS_BILINEAR, nullptr, nullptr, nullptr);

		if (sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
		              dst->data, dst->linesize) < 0) {
			throw std::runtime_error("Could not scale image");
		}
		return dst;
	}

	void onPush(std::function<void(std::shared_ptr<AVFrame>)> callback) {
		std::lock_guard<std::mutex> lock(mutex);
		onPushCallback = callback;
	}
};

std::shared_ptr<MediaStreamTrack> getMediaStreamTrack(const std::string &id);
std::string emplaceMediaStreamTrack(std::shared_ptr<MediaStreamTrack> ptr);
void eraseMediaStreamTrack(const std::string &id);
