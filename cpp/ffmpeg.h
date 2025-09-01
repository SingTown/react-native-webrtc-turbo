#pragma once

#include <mutex>
#include <queue>
#include <vector>

extern "C" {
#define AVMediaType FFmpeg_AVMediaType
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#undef AVMediaType
}

class Decoder {
  private:
	AVCodecContext *ctx;
	std::mutex mutex;

  public:
	Decoder(AVCodecID codecId) {

		auto decoder = avcodec_find_decoder(codecId);
		if (!decoder)
			throw std::runtime_error("Could not find decoder");

		ctx = avcodec_alloc_context3(decoder);
		if (!ctx)
			throw std::runtime_error("Could not allocate AVCodecContext");

		ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
		if (avcodec_open2(ctx, decoder, NULL) < 0)
			throw std::runtime_error("Could not open codec");
	}

	std::vector<std::shared_ptr<AVFrame>>
	decode(std::shared_ptr<AVPacket> packet) {
		std::lock_guard lock(mutex);

		if (avcodec_send_packet(ctx, packet.get()) < 0) {
			throw std::runtime_error("Error sending packet");
		}

		std::vector<std::shared_ptr<AVFrame>> frames;
		while (1) {
			std::shared_ptr<AVFrame> frame(
			    av_frame_alloc(), [](AVFrame *f) { av_frame_free(&f); });
			if (!frame)
				throw std::runtime_error("Could not allocate image");

			int ret = avcodec_receive_frame(ctx, frame.get());
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
				throw std::runtime_error("Error during decoding");

			frames.push_back(frame);
		}
		return frames;
	}

	void flush() { decode(nullptr); }

	~Decoder() {
		flush();
		if (ctx) {
			avcodec_free_context(&ctx);
			ctx = nullptr;
		}
	}
};

inline std::shared_ptr<AVPacket> createAVPacket() {
	return std::shared_ptr<AVPacket>(av_packet_alloc(),
	                                 [](AVPacket *f) { av_packet_free(&f); });
}
inline std::shared_ptr<AVFrame> createAVFrame() {
	return std::shared_ptr<AVFrame>(av_frame_alloc(),
	                                [](AVFrame *f) { av_frame_free(&f); });
}