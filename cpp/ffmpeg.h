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
#include <libavutil/audio_fifo.h>
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
	auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *f) {
		if (f) {
			av_frame_free(&f);
		}
	});
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
	auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *f) {
		if (f) {
			av_frame_free(&f);
		}
	});
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
	SwsContext *sws_ctx = nullptr;
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
	SwrContext *swr_ctx = nullptr;
	std::mutex mutex;
	AVSampleFormat inFormat = AV_SAMPLE_FMT_NONE;
	int inChannels = 0;
	int inSampleRate = 0;
	AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;
	int outChannels = 0;
	int outSampleRate = 0;
	int pts = -1;

	void init(std::shared_ptr<AVFrame> frame, AVSampleFormat outFormat,
	          int outSampleRate, int outChannels) {
		this->inFormat = (AVSampleFormat)frame->format;
		this->inChannels = frame->ch_layout.nb_channels;
		this->inSampleRate = frame->sample_rate;
		this->outFormat = outFormat;
		this->outChannels = outChannels;
		this->outSampleRate = outSampleRate;

		AVChannelLayout outLayout;
		av_channel_layout_default(&outLayout, outChannels);
		int ret = swr_alloc_set_opts2(&swr_ctx, &outLayout, outFormat,
		                              outSampleRate, &frame->ch_layout,
		                              inFormat, inSampleRate, 0, nullptr);
		if (ret < 0) {
			throw std::runtime_error("Could not allocate resampler context");
		}

		ret = swr_init(swr_ctx);
		if (ret < 0) {
			throw std::runtime_error("Could not open resample context");
		}
	}

  public:
	std::shared_ptr<AVFrame> resample(std::shared_ptr<AVFrame> frame,
	                                  AVSampleFormat outFormat,
	                                  int outSampleRate, int outChannels) {
		std::lock_guard lock(mutex);
		if (!swr_ctx && frame) {
			init(frame, outFormat, outSampleRate, outChannels);
		}
		if (!swr_ctx && !frame) {
			return nullptr;
		}
		if (pts < 0 && frame) {
			pts = frame->pts;
		}

		int in_nb_samples = frame ? frame->nb_samples : 0;
		int outSamples = swr_get_out_samples(swr_ctx, in_nb_samples);

		if (outSamples == 0) {
			return nullptr;
		}

		std::shared_ptr<AVFrame> dst;
		int ret = -1;

		if (frame) {
			dst = createAudioFrame(outFormat, frame->pts, outSampleRate,
			                       outChannels, outSamples);

			ret = swr_convert(swr_ctx, dst->data, dst->nb_samples, frame->data,
			                  frame->nb_samples);

		} else {
			dst = createAudioFrame(outFormat, 0, outSampleRate, outChannels,
			                       outSamples);

			ret = swr_convert(swr_ctx, dst->data, dst->nb_samples, nullptr, 0);
		}
		dst->nb_samples = ret;
		if (ret == 0) {
			return nullptr;
		} else if (ret < 0) {
			throw std::runtime_error("Could not convert audio");
		}
		dst->pts = pts;
		pts += ret;
		return dst;
	}
	~Resampler() {
		if (swr_ctx) {
			swr_free(&swr_ctx);
			swr_ctx = nullptr;
		}
	}
};

class AudioFifo {
  private:
	AVAudioFifo *fifo = nullptr;
	std::mutex mutex;
	AVSampleFormat format = AV_SAMPLE_FMT_NONE;
	int channels = 0;
	int sample_rate = 0;
	int64_t pts = -1;

	void init(std::shared_ptr<AVFrame> frame) {
		format = (AVSampleFormat)frame->format;
		channels = frame->ch_layout.nb_channels;
		sample_rate = frame->sample_rate;
		fifo = av_audio_fifo_alloc(format, channels, 1024);
		if (!fifo) {
			throw std::runtime_error("Could not allocate audio fifo");
		}
	}

  public:
	~AudioFifo() {
		if (fifo) {
			av_audio_fifo_free(fifo);
			fifo = nullptr;
		}
	}

	void write(std::shared_ptr<AVFrame> frame) {
		std::lock_guard lock(mutex);
		if (!frame) {
			return;
		}

		if (!fifo && frame) {
			init(frame);
		}

		if (av_audio_fifo_write(fifo, (void **)frame->data, frame->nb_samples) <
		    frame->nb_samples) {
			throw std::runtime_error("Could not write to audio fifo");
		}
		if (pts == -1) {
			pts = frame->pts;
		}
	}

	std::shared_ptr<AVFrame> read(int nb_samples = 960) {
		std::lock_guard lock(mutex);
		if (!fifo) {
			return nullptr;
		}

		if (av_audio_fifo_size(fifo) < nb_samples) {
			return nullptr;
		}

		auto frame =
		    createAudioFrame(format, pts, sample_rate, channels, nb_samples);

		int ret = av_audio_fifo_read(fifo, (void **)frame->data, nb_samples);
		if (ret < 0) {
			throw std::runtime_error("Could not read from audio fifo");
		}

		pts += nb_samples;
		if (av_audio_fifo_size(fifo) == 0) {
			pts = -1;
		}

		return frame;
	}

	void clear() {
		std::lock_guard lock(mutex);
		if (fifo) {
			av_audio_fifo_reset(fifo);
		}
	}
};

class Encoder {
  private:
	Scaler scaler;
	Resampler resampler;
	AudioFifo fifo;
	std::mutex mutex;

	void init(std::shared_ptr<AVFrame> frame) {
		ctx = avcodec_alloc_context3(encoder);
		if (!ctx)
			throw std::runtime_error("Could not allocate AVCodecContext");

		if (encoder->id == AV_CODEC_ID_H264) {
			printf("init H264 ctx\n");
			ctx->codec_id = AV_CODEC_ID_H264;
			ctx->width = frame->width;
			ctx->height = frame->height;
			ctx->time_base = (AVRational){1, 90000};
			ctx->framerate = (AVRational){30, 1};
			ctx->bit_rate = 1000000; // 1Mbps ~ 130KB/s
			ctx->gop_size = 60;
			ctx->max_b_frames = 0;
			ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			ctx->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
			ctx->color_range = AVCOL_RANGE_MPEG;
			ctx->color_primaries = AVCOL_PRI_BT709;
			ctx->color_trc = AVCOL_TRC_BT709;
			ctx->colorspace = AVCOL_SPC_BT709;
		} else if (encoder->id == AV_CODEC_ID_H265) {
			printf("init H265 ctx\n");
			ctx->codec_id = AV_CODEC_ID_H265;
			ctx->width = frame->width;
			ctx->height = frame->height;
			ctx->time_base = (AVRational){1, 90000};
			ctx->framerate = (AVRational){30, 1};
			ctx->bit_rate = 1000000; // 1Mbps ~ 130KB/s
			ctx->gop_size = 60;
			ctx->max_b_frames = 0;
			ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			ctx->profile = FF_PROFILE_HEVC_MAIN;
			ctx->color_range = AVCOL_RANGE_MPEG;
			ctx->color_primaries = AVCOL_PRI_BT709;
			ctx->color_trc = AVCOL_TRC_BT709;
			ctx->colorspace = AVCOL_SPC_BT709;
		} else if (encoder->id == AV_CODEC_ID_OPUS) {
			printf("init opus ctx\n");
			ctx->codec_id = AV_CODEC_ID_OPUS;
			ctx->time_base = (AVRational){1, 48000};
			ctx->sample_rate = 48000;
			ctx->bit_rate = 64000; // 64kbps ~ 8KB/s
			ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
			av_channel_layout_default(&ctx->ch_layout, 2);
		} else if (encoder->id == AV_CODEC_ID_AAC) {
			printf("init aac ctx\n");
			ctx->codec_id = AV_CODEC_ID_AAC;
			ctx->time_base = (AVRational){1, 48000};
			ctx->sample_rate = 48000;
			ctx->bit_rate = 128000; // 128kbps ~ 16KB/s
			ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
			av_channel_layout_default(&ctx->ch_layout, 2);
		} else {
			throw std::runtime_error("Unsupported encoder" +
			                         std::to_string(encoder->id));
		}
		ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
		if (avcodec_open2(ctx, encoder, NULL) < 0)
			throw std::runtime_error("Could not open codec" +
			                         std::string(encoder->name));
	}

	void destroy() {
		if (!ctx)
			return;
		avcodec_free_context(&ctx);
		ctx = nullptr;
	}

  public:
	const AVCodec *encoder = nullptr;
	AVCodecContext *ctx = nullptr;

	Encoder(AVCodecID codecId = AV_CODEC_ID_NONE) {
		std::lock_guard lock(mutex);
		if (codecId == AV_CODEC_ID_NONE) {
			return;
		}
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

	std::vector<std::shared_ptr<AVPacket>>
	encode(std::shared_ptr<AVFrame> frame) {
		std::lock_guard lock(mutex);

		if (!ctx && frame) {
			init(frame);
		}
		if (!ctx && !frame) {
			return {};
		}

		auto frames = std::vector<std::shared_ptr<AVFrame>>();

		if (encoder->id == AV_CODEC_ID_OPUS) {
			auto resampled_frame =
			    resampler.resample(frame, AV_SAMPLE_FMT_FLT, 48000, 2);
			fifo.write(resampled_frame);
			while (auto f = fifo.read(960)) {
				frames.push_back(f);
			}
		} else if (encoder->id == AV_CODEC_ID_AAC) {
			auto resampled_frame =
			    resampler.resample(frame, AV_SAMPLE_FMT_FLTP, 48000, 2);
			fifo.write(resampled_frame);
			while (auto f = fifo.read(1024)) {
				frames.push_back(f);
			}
		} else if (encoder->id == AV_CODEC_ID_H264 ||
		           encoder->id == AV_CODEC_ID_H265) {
			if (frame) {
				frames.push_back(scaler.scale(frame, AV_PIX_FMT_YUV420P,
				                              frame->width, frame->height));
			}
		} else {
			throw std::runtime_error("Unsupported encoder" +
			                         std::to_string(encoder->id));
		}

		if (!frame) {
			frames.push_back(nullptr);
		}

		std::vector<std::shared_ptr<AVPacket>> packets;
		for (auto &f : frames) {
			int ret = avcodec_send_frame(ctx, f.get());
			if (ret < 0) {
				throw std::runtime_error("Error sending frame");
			}

			while (true) {
				auto packet = createAVPacket();
				ret = avcodec_receive_packet(ctx, packet.get());
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;
				else if (ret < 0)
					throw std::runtime_error("Error during decoding");
				packets.push_back(packet);
			}
		}

		return packets;
	}
};

class Decoder {
  private:
	AVCodecContext *ctx = nullptr;
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
