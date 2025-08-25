#include "ffmpeg.h"

Decoder::Decoder(AVCodecID codecId) {
	auto decoder = avcodec_find_decoder(codecId);
	if (!decoder)
		throw std::runtime_error("Could not find decoder");

	codec_ctx = avcodec_alloc_context3(decoder);
	if (!codec_ctx)
		throw std::runtime_error("Could not allocate AVCodecContext");

	codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
	if (avcodec_open2(codec_ctx, decoder, NULL) < 0)
		throw std::runtime_error("Could not open codec");
}

Decoder::~Decoder() {
	flush();
	if (sws_ctx) {
		sws_freeContext(sws_ctx);
		sws_ctx = nullptr;
	}
	if (codec_ctx) {
		avcodec_free_context(&codec_ctx);
		codec_ctx = nullptr;
	}
}

void Decoder::sendPacket(const std::vector<std::byte> &nal) {

	AVPacket *pkt;
	if (nal.empty()) {
		pkt = nullptr;
	} else {
		pkt = av_packet_alloc();
		if (!pkt)
			throw std::runtime_error("Could not allocate AVPacket");

		if (av_new_packet(pkt, nal.size()) < 0) {
			av_packet_free(&pkt);
			throw std::runtime_error("Could not allocate packet data");
		}
		memcpy(pkt->data, nal.data(), nal.size());
	}
	if (avcodec_send_packet(codec_ctx, pkt) < 0) {
		av_packet_free(&pkt);
		throw std::runtime_error("Error sending packet");
	}
	av_packet_free(&pkt);

	AVFrame *frame = av_frame_alloc();
	if (!frame)
		throw std::runtime_error("Could not allocate AVFrame");

	while (1) {

		int ret = avcodec_receive_frame(codec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0)
			throw std::runtime_error("Error during decoding");

		AVPixelFormat format = (AVPixelFormat)frame->format;

		sws_ctx =
		    sws_getCachedContext(sws_ctx, frame->width, frame->height, format,
		                         frame->width, frame->height, AV_PIX_FMT_RGBA,
		                         SWS_BILINEAR, nullptr, nullptr, nullptr);

		RGBAFrame rgbaFrame(frame->width, frame->height);
		uint8_t *dest[4] = {(uint8_t *)rgbaFrame.data.data(), nullptr, nullptr,
		                    nullptr};
		int dest_linesize[4] = {rgbaFrame.linesize, 0, 0, 0};

		if (sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
		              dest, dest_linesize) < 0) {
			av_frame_free(&frame);
			throw std::runtime_error("Could not scale image");
		}

		std::lock_guard lock(frameQueueMutex);
		frameQueue.push(rgbaFrame);
	}
	av_frame_free(&frame);
}

void Decoder::flush() { sendPacket({}); }

std::optional<RGBAFrame> Decoder::receiveFrame() {
	std::lock_guard lock(frameQueueMutex);
	if (frameQueue.empty())
		return std::nullopt;
	auto frame = frameQueue.front();
	frameQueue.pop();
	return frame;
}
