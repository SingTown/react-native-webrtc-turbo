#include "ffmpeg.h"
#include "framepipe.h"
#include <gtest/gtest.h>

TEST(FramePipeTest, testCallback) {
	bool called = false;
	int subscriptionIdSet = -1;
	int subscriptionId = subscribe(
	    {"test_pipe"},
	    [&called, &subscriptionIdSet](std::string pipeId, int subscriptionId,
	                                  std::shared_ptr<AVFrame> frame) {
		    ASSERT_EQ(pipeId, std::string("test_pipe"));
		    ASSERT_NE(frame, nullptr);
		    subscriptionIdSet = subscriptionId;
		    called = true;
	    });

	auto frame = createAudioFrame(AV_SAMPLE_FMT_S16, 48000, 2, 960);

	ASSERT_FALSE(called);
	publish("test_pipe", frame);
	ASSERT_TRUE(called);
	ASSERT_EQ(subscriptionIdSet, subscriptionId);

	unsubscribe(subscriptionId);
}

TEST(FramePipeTest, testCallbackNotMatch) {
	bool called = false;
	int subscriptionId = subscribe(
	    {"test_pipe"}, [&called](std::string pipeId, int subscriptionId,
	                             std::shared_ptr<AVFrame> frame) {
		    ASSERT_NE(frame, nullptr);
		    called = true;
	    });

	auto frame = createAudioFrame(AV_SAMPLE_FMT_S16, 48000, 2, 960);

	ASSERT_FALSE(called);
	publish("test_pipe2", frame);
	ASSERT_FALSE(called);

	unsubscribe(subscriptionId);
}

TEST(FramePipeTest, testCleanup) {
	bool cleanedUp = false;
	int subscriptionIdSet = -1;
	int subscriptionId = subscribe(
	    {"test_pipe"}, [](std::string, int, std::shared_ptr<AVFrame> frame) {},
	    [&cleanedUp, &subscriptionIdSet](int subscriptionId) {
		    cleanedUp = true;
		    subscriptionIdSet = subscriptionId;
	    });

	ASSERT_FALSE(cleanedUp);
	unsubscribe(subscriptionId);
	ASSERT_TRUE(cleanedUp);
	ASSERT_EQ(subscriptionIdSet, subscriptionId);
}

TEST(FramePipeTest, testUnsubscribe) {
	int count = 0;
	int subscriptionId = subscribe(
	    {"test_pipe"}, [&count](std::string pipeId, int subId,
	                            std::shared_ptr<AVFrame>) { count += 1; });

	auto frame = createAudioFrame(AV_SAMPLE_FMT_S16, 48000, 2, 960);
	publish("test_pipe", frame);
	publish("test_pipe", frame);
	publish("test_pipe", frame);
	publish("test_pipe", frame);
	unsubscribe(subscriptionId);
	ASSERT_EQ(count, 4);
}

TEST(FramePipeTest, testUnsubscribeInCallback) {
	int count = 0;
	int subscriptionId =
	    subscribe({"test_pipe"}, [&count](std::string pipeId, int subId,
	                                      std::shared_ptr<AVFrame>) {
		    count += 1;
		    unsubscribe(subId);
	    });

	auto frame = createAudioFrame(AV_SAMPLE_FMT_S16, 48000, 2, 960);
	publish("test_pipe", frame);
	publish("test_pipe", frame);
	publish("test_pipe", frame);
	publish("test_pipe", frame);
	unsubscribe(subscriptionId);
	ASSERT_EQ(count, 1);
}