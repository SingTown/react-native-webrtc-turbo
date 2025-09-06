#pragma once

#include "log.h"
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <vector>

extern "C" {
#define AVMediaType FFmpeg_AVMediaType
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#undef AVMediaType
}

class Encoder {
  private:
	const AVCodec *encoder;
	AVCodecContext *ctx;
	std::mutex mutex;

	void destroy() {
		if (!ctx)
			return;
		avcodec_send_frame(ctx, nullptr);
		AVPacket *pkt = av_packet_alloc();
		while (avcodec_receive_packet(ctx, pkt) == 0) {
			av_packet_unref(pkt);
		}
		av_packet_free(&pkt);
		avcodec_free_context(&ctx);
		ctx = nullptr;
	}

  public:
	Encoder(AVCodecID codecId) : encoder(nullptr), ctx(nullptr) {
		std::lock_guard lock(mutex);
		encoder = avcodec_find_encoder(codecId);
		if (encoder) {
			LOGI("encoder %s enabled\n", encoder->name);
		} else {
			LOGE("encoder init failed\n");
		}
		if (!encoder)
			throw std::runtime_error("Could not find encoder");
	}

	std::vector<std::shared_ptr<AVPacket>>
	encode(std::shared_ptr<AVFrame> frame) {
		std::lock_guard lock(mutex);
		if (!frame) {
			throw std::runtime_error("frame is nullptr");
		}
		if (ctx) {
			if (ctx->width != frame->width || ctx->height != frame->height ||
			    ctx->pix_fmt != frame->format) {
				LOGI("destroy ctx: %p\n", ctx);
				destroy();
			}
		}

		if (!ctx) {
			printf("init ctx\n");
			// init
			ctx = avcodec_alloc_context3(encoder);
			if (!ctx)
				throw std::runtime_error("Could not allocate AVCodecContext");

			ctx->width = frame->width;
			ctx->height = frame->height;
			ctx->time_base = (AVRational){1, 90000};
			ctx->framerate = (AVRational){30, 1};
			ctx->bit_rate = 1000000; // 1Mbps ~ 130KB/s
			ctx->gop_size = 60;
			ctx->max_b_frames = 0;
			ctx->pix_fmt = (AVPixelFormat)frame->format;
			ctx->color_range = AVCOL_RANGE_MPEG;
			ctx->color_primaries = AVCOL_PRI_BT709;
			ctx->color_trc = AVCOL_TRC_BT709;
			ctx->colorspace = AVCOL_SPC_BT709;
			if (encoder->id == AV_CODEC_ID_H264) {
				ctx->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
			} else if (encoder->id == AV_CODEC_ID_H265) {
				ctx->profile = FF_PROFILE_HEVC_MAIN;
			}
			ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
			if (avcodec_open2(ctx, encoder, NULL) < 0)
				throw std::runtime_error("Could not open codec");
		}

		int ret = avcodec_send_frame(ctx, frame.get());
		if (ret < 0) {
			char errbuf[128];
			av_strerror(ret, errbuf, sizeof(errbuf));
			throw std::runtime_error("Error sending frame");
		}

		std::vector<std::shared_ptr<AVPacket>> packets;
		while (1) {
			std::shared_ptr<AVPacket> packet(
			    av_packet_alloc(), [](AVPacket *f) { av_packet_free(&f); });
			if (!packet)
				throw std::runtime_error("Could not allocate AVPacket");

			int ret = avcodec_receive_packet(ctx, packet.get());
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
				throw std::runtime_error("Error during decoding");
			packets.push_back(packet);
		}
		return packets;
	}

	~Encoder() { destroy(); }
};

class Decoder {
  private:
	AVCodecContext *ctx;
	std::mutex mutex;

  public:
	Decoder(AVCodecID codecId) {
		std::lock_guard lock(mutex);
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