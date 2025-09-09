
#pragma once

#include "ffmpeg.h"
#include "guid.h"
#include "log.h"
#include <functional>
#include <mutex>
#include <queue>

struct ComparePTS {
	bool operator()(std::shared_ptr<AVFrame> &a,
	                std::shared_ptr<AVFrame> &b) const {
		return a->pts > b->pts;
	}
};

class MediaStreamTrack {
  private:
	std::priority_queue<std::shared_ptr<AVFrame>,
	                    std::vector<std::shared_ptr<AVFrame>>, ComparePTS>
	    queue;
	std::mutex mutex;
	std::function<void(std::shared_ptr<AVFrame>)> onPushCallback;
	struct SwsContext *sws_ctx;
	int64_t pts;

  public:
	~MediaStreamTrack() {
		onPushCallback = nullptr;
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

	std::shared_ptr<AVFrame> pop(AVPixelFormat format, size_t bufferSize = 0) {
		std::lock_guard<std::mutex> lock(mutex);
		std::shared_ptr<AVFrame> frame;
		while (queue.size() > bufferSize) {
			auto f = queue.top();
			queue.pop();
			if (f->pts < pts) {
				LOGI("drop pts: %lld, last pts: %lld", f->pts, pts);
				continue;
			} else {
				frame = f;
				pts = f->pts;
				break;
			}
		}
		if (!frame) {
			return nullptr;
		}

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
