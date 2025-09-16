#include "ffmpeg.h"
#include <cmath>
#include <gtest/gtest.h>
#include <random>

TEST(CodecTest, testResampleFormat) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 1, 48000, 1, 960);
	float *inData = (float *)inputFrame->data[0];

	for (int i = 0; i < 960; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		inData[i] = dist(rng);
	}

	auto resampler = Resampler();
	auto outFrame = resampler.resample(inputFrame, AV_SAMPLE_FMT_S16, 48000, 1);

	int16_t *outData = (int16_t *)outFrame->data[0];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outData[i], (int16_t)(inData[i] * 32767), 2);
	}
}

TEST(CodecTest, testResamplePlanar) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLTP, 1, 48000, 2, 960);
	float *leftData = (float *)inputFrame->data[0];
	float *rightData = (float *)inputFrame->data[1];

	for (int i = 0; i < 960; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		leftData[i] = dist(rng);
		rightData[i] = dist(rng);
	}

	auto resampler = Resampler();
	auto outFrame =
	    resampler.resample(inputFrame, AV_SAMPLE_FMT_S16P, 48000, 2);

	int16_t *outLeft = (int16_t *)outFrame->data[0];
	int16_t *outRight = (int16_t *)outFrame->data[1];
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outLeft[i], (int16_t)(leftData[i] * 32767), 2);
		EXPECT_NEAR(outRight[i], (int16_t)(rightData[i] * 32767), 2);
	}
}

TEST(CodecTest, testResampleChannels12) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 1, 48000, 1, 960);
	float *inData = (float *)inputFrame->data[0];

	for (int i = 0; i < 960; ++i) {
		static std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		inData[i] = dist(rng);
	}

	auto resampler = Resampler();
	auto outFrame = resampler.resample(inputFrame, AV_SAMPLE_FMT_FLT, 48000, 2);
	ASSERT_EQ(outFrame->format, AV_SAMPLE_FMT_FLT);
	float *outData = (float *)outFrame->data[0];
	ASSERT_EQ(outFrame->nb_samples, 960);
	for (int i = 0; i < 960; ++i) {
		EXPECT_NEAR(outData[i * 2], inData[i] * 0.7071, 0.001);
		EXPECT_NEAR(outData[i * 2 + 1], inData[i] * 0.7071, 0.001);
	}
}

TEST(CodecTest, testResampleRate) {
	auto inputFrame = createAudioFrame(AV_SAMPLE_FMT_FLT, 1, 16000, 1, 320);
	float *inData = (float *)inputFrame->data[0];

	for (int i = 0; i < 320; ++i) {
		inData[i] = i / 319.0f * 2 - 1;
	}
	ASSERT_EQ(inData[0], -1);
	ASSERT_EQ(inData[319], 1);

	auto resampler = Resampler();
	auto outFrame = resampler.resample(inputFrame, AV_SAMPLE_FMT_FLT, 48000, 1);

	float *outData = (float *)outFrame->data[0];
	ASSERT_EQ(outFrame->ch_layout.nb_channels, 1);
	ASSERT_EQ(outFrame->format, AV_SAMPLE_FMT_FLT);
	ASSERT_EQ(outFrame->sample_rate, 48000);
	for (int i = 0; i < outFrame->nb_samples; ++i) {
		EXPECT_NEAR(outData[i], inData[i / 3], 0.01);
	}
}
