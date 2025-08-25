#pragma once

#include <mutex>
#include <queue>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class RGBAFrame {
  public:
	std::vector<std::byte> data;
	int width;
	int height;
	int linesize;
	RGBAFrame(int width, int height) : width(width), height(height) {
		const int rgbaSize =
		    av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
		int dest_linesize[4];
		int ret =
		    av_image_fill_linesizes(dest_linesize, AV_PIX_FMT_RGBA, width);
		if (ret < 0) {
			throw std::runtime_error("Could not fill image line sizes");
		}
		linesize = dest_linesize[0];
		data.resize(rgbaSize);
	}
};

class Decoder {
  public:
	Decoder(AVCodecID codecId);
	~Decoder();

	void sendPacket(const std::vector<std::byte> &nal);
	std::optional<RGBAFrame> receiveFrame();
	void flush();

  private:
	AVCodecContext *codec_ctx;
	struct SwsContext *sws_ctx;
	std::queue<RGBAFrame> frameQueue;
	std::mutex frameQueueMutex;
};