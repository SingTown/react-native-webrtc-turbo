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
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#undef AVMediaType
}

inline std::shared_ptr<AVPacket> createAVPacket() {
	return std::shared_ptr<AVPacket>(av_packet_alloc(),
	                                 [](AVPacket *f) { av_packet_free(&f); });
}

inline std::shared_ptr<AVFrame> createVideoFrame(AVPixelFormat format, int pts,
                                                 int width, int height) {
	auto frame = std::shared_ptr<AVFrame>(
	    av_frame_alloc(), [](AVFrame *f) { av_frame_free(&f); });
	if (!frame) {
		throw std::runtime_error("Could not allocate AVFrame");
	}
	frame->format = format;
	frame->pts = pts;
	frame->width = width;
	frame->height = height;

	if (av_frame_get_buffer(frame.get(), 32) < 0) {
		throw std::runtime_error("Could not allocate AVFrame");
	}
	return frame;
}

inline std::shared_ptr<AVFrame> createAudioFrame(AVSampleFormat format, int pts,
                                                 int sampleRate, int channels,
                                                 int nb_samples) {
	auto frame = std::shared_ptr<AVFrame>(
	    av_frame_alloc(), [](AVFrame *f) { av_frame_free(&f); });
	if (!frame) {
		throw std::runtime_error("Could not allocate AVFrame");
	}
	frame->format = format;
	frame->pts = pts;
	frame->sample_rate = sampleRate;
	frame->nb_samples = nb_samples;
	av_channel_layout_default(&frame->ch_layout, channels);

	if (av_frame_get_buffer(frame.get(), 0) < 0) {
		throw std::runtime_error("Could not allocate AVFrame");
	}
	return frame;
}

class Scaler {
  private:
	SwsContext *sws_ctx;
	std::mutex mutex;

  public:
	std::shared_ptr<AVFrame> scale(std::shared_ptr<AVFrame> frame,
	                               AVPixelFormat format, int width,
	                               int height) {
		std::lock_guard lock(mutex);
		if (!frame) {
			return nullptr;
		}
		if (frame->format == format && frame->width == width &&
		    frame->height == height) {
			return frame;
		}

		auto dst = createVideoFrame(format, frame->pts, width, height);
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
	~Scaler() {
		if (sws_ctx) {
			sws_freeContext(sws_ctx);
			sws_ctx = nullptr;
		}
	}
};

class Resampler {
  private:
	SwrContext *swr_ctx;
	std::mutex mutex;
	AVSampleFormat inFormat = AV_SAMPLE_FMT_NONE;
	int inChannels = 0;
	int inSampleRate = 0;
	AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;
	int outChannels = 0;
	int outSampleRate = 0;

  public:
	std::shared_ptr<AVFrame> resample(std::shared_ptr<AVFrame> frame,
	                                  AVSampleFormat outFormat,
	                                  int outSampleRate, int outChannels) {
		std::lock_guard lock(mutex);

		if (!frame) {
			return nullptr;
		}
		AVSampleFormat inFormat = (AVSampleFormat)frame->format;
		int inChannels = frame->ch_layout.nb_channels;
		int inSampleRate = frame->sample_rate;

		if (inFormat == outFormat && inSampleRate == outSampleRate &&
		    inChannels == outChannels) {
			return frame;
		}

		if (this->inFormat != inFormat || this->inChannels != inChannels ||
		    this->inSampleRate != inSampleRate ||
		    this->outFormat != outFormat || this->outChannels != outChannels ||
		    this->outSampleRate != outSampleRate) {
			if (swr_ctx) {
				swr_free(&swr_ctx);
				swr_ctx = nullptr;
			}
		}
		if (!swr_ctx) {
			AVChannelLayout outLayout;
			av_channel_layout_default(&outLayout, outChannels);
			int ret = swr_alloc_set_opts2(&swr_ctx, &outLayout, outFormat,
			                              outSampleRate, &frame->ch_layout,
			                              inFormat, inSampleRate, 0, nullptr);
			if (ret < 0) {
				throw std::runtime_error(
				    "Could not allocate resampler context");
			}

			ret = swr_init(swr_ctx);
			if (ret < 0) {
				throw std::runtime_error("Could not open resample context");
			}
			this->inFormat = inFormat;
			this->inChannels = inChannels;
			this->inSampleRate = inSampleRate;
			this->outFormat = outFormat;
			this->outChannels = outChannels;
			this->outSampleRate = outSampleRate;
		}

		int outSamples = swr_get_out_samples(swr_ctx, frame->nb_samples);
		auto dst = createAudioFrame(outFormat, frame->pts, outSampleRate,
		                            outChannels, outSamples);

		int ret = swr_convert(swr_ctx, dst->data, dst->nb_samples, frame->data,
		                      frame->nb_samples);
		dst->nb_samples = ret;
		if (ret < 0) {
			throw std::runtime_error("Could not convert audio");
		}
		return dst;
	}
	~Resampler() {
		if (swr_ctx) {
			swr_free(&swr_ctx);
			swr_ctx = nullptr;
		}
	}
};

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
	std::vector<std::shared_ptr<AVPacket>>
	encode(std::shared_ptr<AVFrame> frame) {
		std::lock_guard lock(mutex);

		if (ctx && frame) {
			if (encoder->type == AVMEDIA_TYPE_VIDEO &&
			    (ctx->width != frame->width || ctx->height != frame->height ||
			     ctx->pix_fmt != frame->format)) {
				LOGI("destroy ctx: %p\n", ctx);
				destroy();
			} else if (encoder->type == AVMEDIA_TYPE_AUDIO &&
			           (ctx->sample_rate != frame->sample_rate ||
			            av_channel_layout_compare(&ctx->ch_layout,
			                                      &frame->ch_layout) != 0 ||
			            ctx->sample_fmt != (AVSampleFormat)frame->format)) {
				LOGI("destroy ctx: %p\n", ctx);
				destroy();
			}
		}

		if (!ctx && frame) {
			// init
			ctx = avcodec_alloc_context3(encoder);
			if (!ctx)
				throw std::runtime_error("Could not allocate AVCodecContext");

			if (encoder->id == AV_CODEC_ID_H264) {
				printf("init H264 ctx\n");
				ctx->width = frame->width;
				ctx->height = frame->height;
				ctx->time_base = (AVRational){1, 90000};
				ctx->framerate = (AVRational){30, 1};
				ctx->bit_rate = 1000000; // 1Mbps ~ 130KB/s
				ctx->gop_size = 60;
				ctx->max_b_frames = 0;
				ctx->pix_fmt = (AVPixelFormat)frame->format;
				ctx->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
				ctx->color_range = AVCOL_RANGE_MPEG;
				ctx->color_primaries = AVCOL_PRI_BT709;
				ctx->color_trc = AVCOL_TRC_BT709;
				ctx->colorspace = AVCOL_SPC_BT709;
			} else if (encoder->id == AV_CODEC_ID_H265) {
				printf("init H265 ctx\n");
				ctx->width = frame->width;
				ctx->height = frame->height;
				ctx->time_base = (AVRational){1, 90000};
				ctx->framerate = (AVRational){30, 1};
				ctx->bit_rate = 1000000; // 1Mbps ~ 130KB/s
				ctx->gop_size = 60;
				ctx->max_b_frames = 0;
				ctx->pix_fmt = (AVPixelFormat)frame->format;
				ctx->profile = FF_PROFILE_HEVC_MAIN;
				ctx->color_range = AVCOL_RANGE_MPEG;
				ctx->color_primaries = AVCOL_PRI_BT709;
				ctx->color_trc = AVCOL_TRC_BT709;
				ctx->colorspace = AVCOL_SPC_BT709;
			} else if (encoder->id == AV_CODEC_ID_OPUS) {
				printf("init opus ctx\n");
				ctx->time_base = (AVRational){1, 48000};
				ctx->sample_rate = 48000;
				ctx->bit_rate = 64000; // 64kbps ~ 8KB/s
				ctx->sample_fmt = (AVSampleFormat)frame->format;
				if (av_channel_layout_copy(&ctx->ch_layout, &frame->ch_layout) <
				    0) {
					throw std::runtime_error("Could not copy channel layout");
				}
			} else {
				throw std::runtime_error("Unsupported encoder" +
				                         std::to_string(encoder->id));
			}
			ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
			if (avcodec_open2(ctx, encoder, NULL) < 0)
				throw std::runtime_error("Could not open codec" +
				                         std::string(encoder->name));
		}

		int ret = avcodec_send_frame(ctx, frame.get());
		if (ret < 0) {
			throw std::runtime_error("Error sending frame");
		}

		std::vector<std::shared_ptr<AVPacket>> packets;
		while (1) {
			auto packet = createAVPacket();
			int ret = avcodec_receive_packet(ctx, packet.get());
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
				throw std::runtime_error("Error during decoding");
			packets.push_back(packet);
		}
		return packets;
	}

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
		if (decoder->id == AV_CODEC_ID_OPUS) {
			printf("init opus ctx\n");
			ctx->time_base = (AVRational){1, 48000};
			ctx->sample_rate = 48000;
			ctx->bit_rate = 64000;
			ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
			av_channel_layout_default(&ctx->ch_layout, 2);
		}
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
