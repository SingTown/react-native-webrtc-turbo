#include "ffmpeg.h"
#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <random>

static void fillNoise(std::shared_ptr<AVFrame> frame) {
	static std::mt19937 rng(std::random_device{}());
	if (frame->format == AV_SAMPLE_FMT_FLT) {
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		int channels = frame->ch_layout.nb_channels;
		float *inData = (float *)frame->data[0];
		for (int i = 0; i < frame->nb_samples * channels; ++i) {
			inData[i] = dist(rng);
		}
	} else if (frame->format == AV_SAMPLE_FMT_FLTP) {
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		int channels = frame->ch_layout.nb_channels;
		for (int ch = 0; ch < channels; ++ch) {
			float *inData = (float *)frame->data[ch];
			for (int i = 0; i < frame->nb_samples; ++i) {
				inData[i] = dist(rng);
			}
		}
	} else if (frame->format == AV_SAMPLE_FMT_S16) {
		static std::uniform_int_distribution<int16_t> dist(-32768, 32767);
		int channels = frame->ch_layout.nb_channels;
		int16_t *inData = (int16_t *)frame->data[0];
		for (int i = 0; i < frame->nb_samples * channels; ++i) {
			inData[i] = dist(rng);
		}
	} else if (frame->format == AV_SAMPLE_FMT_S16P) {
		static std::uniform_int_distribution<int16_t> dist(-32768, 32767);
		int channels = frame->ch_layout.nb_channels;
		for (int ch = 0; ch < channels; ++ch) {
			int16_t *inData = (int16_t *)frame->data[ch];
			for (int i = 0; i < frame->nb_samples; ++i) {
				inData[i] = dist(rng);
			}
		}
	} else {
		throw std::runtime_error("Unsupported format in fillNoise");
	}
}

static void delay(int milliseconds) {
	auto target = globalBaseTime + std::chrono::milliseconds(milliseconds);
	while (std::chrono::high_resolution_clock::now() < target) {
	}
}

TEST(CurrentPtsTest, test90k) {
	resetBaseTime();

	auto tb = (AVRational){1, 90000};
	auto pts1 = currentPts(tb);

	delay(10);

	auto pts2 = currentPts(tb);
	ASSERT_NEAR(pts1, 0, 1);
	ASSERT_NEAR(pts2, 900, 1);
}

TEST(CurrentPtsTest, test48K) {
	resetBaseTime();

	auto tb = (AVRational){1, 48000};
	auto pts1 = currentPts(tb);

	delay(10);

	auto pts2 = currentPts(tb);
	ASSERT_NEAR(pts1, 0, 1);
	ASSERT_NEAR(pts2, 480, 1);
}

TEST(CreateAudioFrameTest, testCreateAudioFrame) {
	auto frame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 2, 960);
	ASSERT_EQ(frame->format, AV_SAMPLE_FMT_FLT);
	ASSERT_EQ(frame->sample_rate, 48000);
	ASSERT_EQ(frame->ch_layout.nb_channels, 2);
	ASSERT_EQ(frame->nb_samples, 960);
}

TEST(CreateVideoFrameTest, testCreateVideoFrame) {
	auto frame = createVideoFrame(AV_PIX_FMT_YUV420P, 1920, 1080);
	ASSERT_EQ(frame->format, AV_PIX_FMT_YUV420P);
	ASSERT_EQ(frame->width, 1920);
	ASSERT_EQ(frame->height, 1080);
}

TEST(PtsTest, testCreateAudioFramePts) {
	resetBaseTime();

	auto frame1 = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 2, 1024);

	delay(10);

	auto frame2 = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 2, 1024);

	ASSERT_NEAR(frame1->pts, 0, 1);
	ASSERT_NEAR(frame2->pts, 480, 10);
}

TEST(PtsTest, testCreateVideoFramePts) {
	resetBaseTime();

	auto frame1 = createVideoFrame(AV_PIX_FMT_YUV420P, 1920, 1080);

	delay(10);

	auto frame2 = createVideoFrame(AV_PIX_FMT_YUV420P, 1920, 1080);

	ASSERT_EQ(frame1->pts, 0);
	ASSERT_NEAR(frame2->pts, 900, 10);
}

TEST(AudioFifoTest, testNomal) {
	resetBaseTime();

	AudioFifo fifo;
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 960);
	fillNoise(inputFrame);

	fifo.write(inputFrame);
	auto outFrame = fifo.read(960);
	ASSERT_EQ(outFrame->nb_samples, 960);
	ASSERT_EQ(outFrame->format, AV_SAMPLE_FMT_FLT);
	ASSERT_EQ(outFrame->sample_rate, 48000);
	ASSERT_EQ(outFrame->ch_layout.nb_channels, 1);
}

TEST(AudioFifoTest, testNull) {
	AudioFifo fifo;
	fifo.write(nullptr);
	auto outFrame = fifo.read(960);
	ASSERT_EQ(outFrame, nullptr);
}

TEST(AudioFifoTest, testPts) {
	AudioFifo fifo;
	auto in1 = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 960, 1);
	auto in2 = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 960, 961);
	fifo.write(in1);
	fifo.write(in2);
	auto out1 = fifo.read(960);
	ASSERT_EQ(out1->pts, 1);
	auto out2 = fifo.read(960);
	ASSERT_EQ(out2->pts, 961);
}

TEST(ResamplerTest, testResampleFormat) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 960);
	fillNoise(inputFrame);

	Resampler resampler;
	AudioFifo fifo;

	fifo.write(resampler.resample(inputFrame, AV_SAMPLE_FMT_S16, 48000, 1));
	fifo.write(resampler.resample(nullptr, AV_SAMPLE_FMT_S16, 48000, 1));

	auto frame = fifo.read(960);
	ASSERT_EQ(frame->format, AV_SAMPLE_FMT_S16);
	ASSERT_EQ(frame->nb_samples, 960);

	float *inData = (float *)inputFrame->data[0];
	int16_t *outData = (int16_t *)frame->data[0];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outData[i] / 32767.0, inData[i], 0.01);
	}
}

TEST(ResamplerTest, testNull) {
	Resampler resampler;
	auto outFrame = resampler.resample(nullptr, AV_SAMPLE_FMT_S16, 48000, 1);
	ASSERT_EQ(outFrame, nullptr);
}

TEST(ResamplerTest, testResamplePlanar) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLTP, 48000, 2, 960);
	fillNoise(inputFrame);

	Resampler resampler;
	AudioFifo fifo;

	fifo.write(resampler.resample(inputFrame, AV_SAMPLE_FMT_S16P, 48000, 2));
	fifo.write(resampler.resample(nullptr, AV_SAMPLE_FMT_S16P, 48000, 2));

	auto outFrame = fifo.read(960);
	ASSERT_EQ(outFrame->format, AV_SAMPLE_FMT_S16P);
	ASSERT_EQ(outFrame->nb_samples, 960);

	float *leftData = (float *)inputFrame->data[0];
	float *rightData = (float *)inputFrame->data[1];
	int16_t *outLeft = (int16_t *)outFrame->data[0];
	int16_t *outRight = (int16_t *)outFrame->data[1];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outLeft[i] / 32767.0, leftData[i], 0.01);
		EXPECT_NEAR(outRight[i] / 32767.0, rightData[i], 0.01);
	}
}

TEST(ResamplerTest, testResampleChannels12) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 960);
	fillNoise(inputFrame);

	Resampler resampler;
	AudioFifo fifo;

	fifo.write(resampler.resample(inputFrame, AV_SAMPLE_FMT_FLT, 48000, 2));
	fifo.write(resampler.resample(nullptr, AV_SAMPLE_FMT_FLT, 48000, 2));

	auto outFrame = fifo.read(960);
	ASSERT_EQ(outFrame->format, AV_SAMPLE_FMT_FLT);
	ASSERT_EQ(outFrame->nb_samples, 960);

	float *inData = (float *)inputFrame->data[0];
	float *outData = (float *)outFrame->data[0];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outData[i * 2], inData[i] * 0.7071, 0.001);
		EXPECT_NEAR(outData[i * 2 + 1], inData[i] * 0.7071, 0.001);
	}
}

TEST(ResamplerTest, testResampleRate) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 16000, 1, 320);
	float *inData = (float *)inputFrame->data[0];

	for (int i = 0; i < 320; ++i) {
		inData[i] = i / 319.0f * 2 - 1;
	}
	ASSERT_EQ(inData[0], -1);
	ASSERT_EQ(inData[319], 1);

	Resampler resampler;
	AudioFifo fifo;

	fifo.write(resampler.resample(inputFrame, AV_SAMPLE_FMT_FLT, 48000, 1));
	fifo.write(resampler.resample(nullptr, AV_SAMPLE_FMT_FLT, 48000, 1));
	auto outFrame = fifo.read(960);
	ASSERT_EQ(outFrame->nb_samples, 960);
	ASSERT_EQ(outFrame->ch_layout.nb_channels, 1);
	ASSERT_EQ(outFrame->format, AV_SAMPLE_FMT_FLT);
	ASSERT_EQ(outFrame->sample_rate, 48000);

	float *outData = (float *)outFrame->data[0];
	for (int i = 0; i < outFrame->nb_samples; ++i) {
		EXPECT_NEAR(outData[i], inData[i / 3], 0.01);
	}
}

TEST(ResamplerTest, testPts) {
	auto in1 = createAudioFrame(AV_SAMPLE_FMT_FLT, 16000, 1, 320, 1);
	auto in2 = createAudioFrame(AV_SAMPLE_FMT_FLT, 16000, 1, 320, 321);
	fillNoise(in1);
	fillNoise(in2);

	auto resampler = Resampler();
	auto out1 = resampler.resample(in1, AV_SAMPLE_FMT_FLT, 48000, 1);
	auto out2 = resampler.resample(in2, AV_SAMPLE_FMT_FLT, 48000, 1);
	ASSERT_EQ(out1->pts, 1);
	ASSERT_EQ(out2->pts, out1->nb_samples + 1);
}

TEST(ScalerTest, test_RGB24_to_NV12) {
	Scaler scaler;
	auto inputFrame = createVideoFrame(AV_PIX_FMT_RGB24, 640, 480);
	auto outFrame = scaler.scale(inputFrame, AV_PIX_FMT_NV12, 320, 240);

	ASSERT_EQ(outFrame->format, AV_PIX_FMT_NV12);
	ASSERT_EQ(outFrame->width, 320);
	ASSERT_EQ(outFrame->height, 240);
	ASSERT_EQ(outFrame->pts, inputFrame->pts);
}

TEST(ScalerTest, test_NV12_to_RGB24) {
	Scaler scaler;
	auto inputFrame = createVideoFrame(AV_PIX_FMT_NV12, 640, 480);
	auto outFrame = scaler.scale(inputFrame, AV_PIX_FMT_RGB24, 320, 240);

	ASSERT_EQ(outFrame->format, AV_PIX_FMT_RGB24);
	ASSERT_EQ(outFrame->width, 320);
	ASSERT_EQ(outFrame->height, 240);
	ASSERT_EQ(outFrame->pts, inputFrame->pts);
}

TEST(EncoderTest, testEncodeOpus) {
	Encoder encoder(AV_CODEC_ID_OPUS);
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLTP, 48000, 2, 960);
	fillNoise(inputFrame);

	std::vector<std::shared_ptr<AVPacket>> packets;
	for (auto &pkt : encoder.encode(inputFrame)) {
		packets.push_back(pkt);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		packets.push_back(pkt);
	}

	ASSERT_GT(packets.size(), 0);
}

TEST(EncoderTest, testEncodeOpusBigFrame) {
	Encoder encoder(AV_CODEC_ID_OPUS);
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLTP, 48000, 2, 960000);
	fillNoise(inputFrame);

	std::vector<std::shared_ptr<AVPacket>> packets;
	for (auto &pkt : encoder.encode(inputFrame)) {
		packets.push_back(pkt);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		packets.push_back(pkt);
	}

	ASSERT_GT(packets.size(), 0);
}

TEST(EncoderTest, testEncodeH264) {
	Encoder encoder(AV_CODEC_ID_H264);
	auto inputFrame = createVideoFrame(AV_PIX_FMT_NV12, 640, 480);

	std::vector<std::shared_ptr<AVPacket>> packets;
	for (auto &pkt : encoder.encode(inputFrame)) {
		packets.push_back(pkt);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		packets.push_back(pkt);
	}

	ASSERT_GT(packets.size(), 0);
}

TEST(EncoderTest, testEncodeH265) {
	Encoder encoder(AV_CODEC_ID_H265);
	auto inputFrame = createVideoFrame(AV_PIX_FMT_NV12, 640, 480);

	std::vector<std::shared_ptr<AVPacket>> packets;
	for (auto &pkt : encoder.encode(inputFrame)) {
		packets.push_back(pkt);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		packets.push_back(pkt);
	}

	ASSERT_GT(packets.size(), 0);
}

TEST(EncoderTest, testEncodeAAC) {
	Encoder encoder(AV_CODEC_ID_AAC);
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 1024);
	fillNoise(inputFrame);

	std::vector<std::shared_ptr<AVPacket>> packets;
	for (auto &pkt : encoder.encode(inputFrame)) {
		packets.push_back(pkt);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		packets.push_back(pkt);
	}

	ASSERT_GT(packets.size(), 0);
}

TEST(EncoderTest, testEncodeAACBigFrame) {
	Encoder encoder(AV_CODEC_ID_AAC);
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 1024000);
	fillNoise(inputFrame);

	std::vector<std::shared_ptr<AVPacket>> packets;
	for (auto &pkt : encoder.encode(inputFrame)) {
		packets.push_back(pkt);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		packets.push_back(pkt);
	}

	ASSERT_GT(packets.size(), 0);
}

TEST(EncoderTest, testEncodePng) {
	Encoder encoder(AV_CODEC_ID_PNG);
	auto inputFrame = createVideoFrame(AV_PIX_FMT_NV12, 640, 480);
	std::string file = testing::TempDir() + "out.png";
	// std::string file = "out.png";
	FILE *f = fopen(file.c_str(), "wb");
	for (auto &pkt : encoder.encode(inputFrame)) {
		fwrite(pkt->data, 1, pkt->size, f);
	}
	for (auto &pkt : encoder.encode(nullptr)) {
		fwrite(pkt->data, 1, pkt->size, f);
	}
	fclose(f);

	ASSERT_TRUE(std::filesystem::exists(file));
}

TEST(MuxerTest, testMuxerAAC) {
	std::string file = testing::TempDir() + "/test_file.mp4";
	Muxer muxer(file, AV_CODEC_ID_AAC, AV_CODEC_ID_NONE);
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 2048);
	fillNoise(inputFrame);

	muxer.mux_audio(inputFrame);
	muxer.stop();
	EXPECT_TRUE(std::filesystem::exists(file));
}

TEST(MuxerTest, testMuxerH264) {
	std::string file = testing::TempDir() + "/test_file.mp4";
	Muxer muxer(file, AV_CODEC_ID_NONE, AV_CODEC_ID_H264);
	auto inputFrame = createVideoFrame(AV_PIX_FMT_NV12, 640, 480);

	muxer.mux_video(inputFrame);
	muxer.stop();
	EXPECT_TRUE(std::filesystem::exists(file));
}

TEST(MuxerTest, testMuxerVideo) {
	std::string file = testing::TempDir() + "/test_file.mp4";
	Muxer muxer(file, AV_CODEC_ID_AAC, AV_CODEC_ID_H264);
	auto in1 = createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 2048);
	auto in2 = createVideoFrame(AV_PIX_FMT_NV12, 640, 480);
	fillNoise(in1);

	muxer.mux_audio(in1);
	muxer.mux_video(in2);
	muxer.stop();
	EXPECT_TRUE(std::filesystem::exists(file));
}

TEST(MuxerTest, testSaveMp4) {
	std::string file = "output.mp4";
	Muxer muxer(file, AV_CODEC_ID_AAC, AV_CODEC_ID_H264);
	for (int i = 0; i < 1000; ++i) {
		auto in1 =
		    createAudioFrame(AV_SAMPLE_FMT_FLT, 48000, 1, 2048, i * 2048);
		fillNoise(in1);
		auto in2 = createVideoFrame(AV_PIX_FMT_NV12, 640, 480, i * 3000);
		muxer.mux_audio(in1);
		muxer.mux_video(in2);
	}
	muxer.stop();
	EXPECT_TRUE(std::filesystem::exists(file));
}