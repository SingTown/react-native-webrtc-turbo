#include "ffmpeg.h"
#include "framepipe.h"
#include <gtest/gtest.h>

TEST(FramePipeTest, testCallback) {
	bool called = false;
	int subscriptionId =
	    subscribe("test_pipe", [&called](std::shared_ptr<AVFrame> frame) {
		    ASSERT_NE(frame, nullptr);
		    called = true;
	    });

	auto frame = createAudioFrame(AV_SAMPLE_FMT_S16, 1, 48000, 2, 960);

	ASSERT_FALSE(called);
	publish("test_pipe", frame);
	ASSERT_TRUE(called);

	unsubscribe(subscriptionId);
}

TEST(FramePipeTest, testCleanup) {
	bool cleanedUp = false;
	int subscriptionId = subscribe(
	    "test_pipe", [](std::shared_ptr<AVFrame> frame) {},
	    [&cleanedUp]() { cleanedUp = true; });

	ASSERT_FALSE(cleanedUp);
	unsubscribe(subscriptionId);
	ASSERT_TRUE(cleanedUp);
}