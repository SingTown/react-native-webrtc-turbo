#include "MediaContainer.h"
#include <gtest/gtest.h>

TEST(MediaContainerTest, testVideo) {
	auto frame1 = createVideoFrame(AV_PIX_FMT_NV12, 1, 640, 480);
	auto frame2 = createVideoFrame(AV_PIX_FMT_NV12, 2, 640, 480);
	auto container = std::make_shared<VideoContainer>();
	container->push(frame1);
	container->push(frame2);
	EXPECT_EQ(container->popVideo(AV_PIX_FMT_NV12)->pts, 1);
	EXPECT_EQ(container->popVideo(AV_PIX_FMT_NV12)->pts, 2);
	EXPECT_EQ(container->popVideo(AV_PIX_FMT_NV12), nullptr);
}

TEST(MediaContainerTest, testVideoPts) {
	auto frame1 = createVideoFrame(AV_PIX_FMT_NV12, 1, 640, 480);
	auto frame2 = createVideoFrame(AV_PIX_FMT_NV12, 2, 640, 480);
	auto container = std::make_shared<VideoContainer>();
	container->push(frame2);
	container->push(frame1);
	EXPECT_EQ(container->popVideo(AV_PIX_FMT_NV12)->pts, 1);
	EXPECT_EQ(container->popVideo(AV_PIX_FMT_NV12)->pts, 2);
	EXPECT_EQ(container->popVideo(AV_PIX_FMT_NV12), nullptr);
}

TEST(MediaContainerTest, testAudioMono) {
	auto frame = createAudioFrame(AV_SAMPLE_FMT_FLT, 1, 48000, 1, 960);
	float *inData = (float *)frame->data[0];
	for (int i = 0; i < 960; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		inData[i] = dist(rng);
	}
	auto track = std::make_shared<AudioContainer>();
	track->push(frame);
	auto frameOut = track->popAudio(AV_SAMPLE_FMT_FLT, 48000, 1);
	EXPECT_EQ(frameOut->nb_samples, 960);
	EXPECT_EQ(frameOut->format, AV_SAMPLE_FMT_FLT);
	EXPECT_EQ(frameOut->sample_rate, 48000);
	EXPECT_EQ(frameOut->ch_layout.nb_channels, 1);
	EXPECT_EQ(frameOut->pts, 1);
	float *outData = (float *)frameOut->data[0];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(inData[i], outData[i], 0.001);
	}
	EXPECT_EQ(track->popAudio(AV_SAMPLE_FMT_FLT, 48000, 1), nullptr);
}

TEST(MediaContainerTest, testAudioStereo) {
	auto frame = createAudioFrame(AV_SAMPLE_FMT_FLT, 1, 48000, 1, 960);
	float *inData = (float *)frame->data[0];
	for (int i = 0; i < 960; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		inData[i] = dist(rng);
	}
	auto track = std::make_shared<AudioContainer>();
	track->push(frame);
	auto frameOut = track->popAudio(AV_SAMPLE_FMT_FLT, 48000, 2);
	EXPECT_EQ(frameOut->nb_samples, 960);
	EXPECT_EQ(frameOut->format, AV_SAMPLE_FMT_FLT);
	EXPECT_EQ(frameOut->sample_rate, 48000);
	EXPECT_EQ(frameOut->ch_layout.nb_channels, 2);
	EXPECT_EQ(frameOut->pts, 1);
	float *outData = (float *)frameOut->data[0];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outData[i * 2], inData[i] * 0.7071, 0.001);
		EXPECT_NEAR(outData[i * 2 + 1], inData[i] * 0.7071, 0.001);
	}
	EXPECT_EQ(track->popAudio(AV_SAMPLE_FMT_FLT, 48000, 2), nullptr);
}

TEST(MediaContainerTest, testAudioMulti) {
	auto frame1 = createAudioFrame(AV_SAMPLE_FMT_FLT, 1, 48000, 1, 800);
	float *inData = (float *)frame1->data[0];
	for (int i = 0; i < frame1->nb_samples; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		inData[i] = dist(rng);
	}

	auto frame2 = createAudioFrame(AV_SAMPLE_FMT_FLT, 801, 48000, 1, 960);
	float *inData2 = (float *)frame2->data[0];
	for (int i = 0; i < frame2->nb_samples; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		inData2[i] = dist(rng);
	}

	auto track = std::make_shared<AudioContainer>();
	track->push(frame1);
	track->push(frame2);
	auto frameOut = track->popAudio(AV_SAMPLE_FMT_FLT, 48000, 1);
	EXPECT_EQ(frameOut->nb_samples, 960);
	EXPECT_EQ(frameOut->format, AV_SAMPLE_FMT_FLT);
	EXPECT_EQ(frameOut->sample_rate, 48000);
	EXPECT_EQ(frameOut->pts, 1);
	float *outData = (float *)frameOut->data[0];
	for (int i = 0; i < 800; ++i) {
		EXPECT_NEAR(inData[i], outData[i], 0.001);
	}
	for (int i = 0; i < 160; ++i) {
		EXPECT_NEAR(inData2[i], outData[i + 800], 0.001);
	}
	EXPECT_EQ(track->popAudio(AV_SAMPLE_FMT_FLT, 48000, 1), nullptr);
}
