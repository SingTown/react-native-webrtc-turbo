#include "RTCRtpReceiver.h"
#include "negotiate.h"
#include <gtest/gtest.h>

TEST(NegotiateTest, extractFmtpStringValue) {
	EXPECT_EQ(extractFmtpStringValue("level-asymmetry-allowed=1",
	                                 "level-asymmetry-allowed"),
	          "1");
	EXPECT_EQ(
	    extractFmtpStringValue("level-asymmetry-allowed=1", "level-asymmetry-"),
	    "");
	EXPECT_EQ(
	    extractFmtpStringValue("packetization-mode=0", "packetization-mode"),
	    "0");
}

TEST(NegotiateTest, getFmtpsProfileLevelId) {
	EXPECT_EQ(getFmtpsProfileLevelId({"level-asymmetry-allowed=1",
	                                  "packetization-mode=1",
	                                  "profile-level-id=4d001f"}),
	          0x4d001f);
	EXPECT_EQ(getFmtpsProfileLevelId({"level-asymmetry-allowed=1",
	                                  "packetization-mode=1",
	                                  "profile-level-id=invalid"}),
	          0x42e01f);
}

TEST(NegotiateTest, getFmtpsPacketizationMode) {
	EXPECT_EQ(getFmtpsPacketizationMode({"level-asymmetry-allowed=1",
	                                     "packetization-mode=1",
	                                     "profile-level-id=4d001f"}),
	          1);
	EXPECT_EQ(getFmtpsPacketizationMode(
	              {"level-asymmetry-allowed=1", "profile-level-id=4d001f"}),
	          0);
}

TEST(NegotiateTest, getFmtpsLevelAsymmetryAllowed) {
	EXPECT_EQ(getFmtpsLevelAsymmetryAllowed({"level-asymmetry-allowed=1",
	                                         "packetization-mode=1",
	                                         "profile-level-id=4d001f"}),
	          true);
	EXPECT_EQ(getFmtpsLevelAsymmetryAllowed(
	              {"packetization-mode=1", "profile-level-id=4d001f"}),
	          false);
}

TEST(NegotiateTest, getProfileId) {
	EXPECT_EQ(getProfileId(0x4d001f), 0x4d);
	EXPECT_EQ(getProfileId(0x42e01f), 0x42);
}

TEST(NegotiateTest, getConstrainedId) {
	EXPECT_EQ(getConstrainedId(0x4d001f), 0x00);
	EXPECT_EQ(getConstrainedId(0x42e01f), 0xe0);
}

TEST(NegotiateTest, getLevelId) {
	EXPECT_EQ(getLevelId(0x4d001f), 0x1f);
	EXPECT_EQ(getLevelId(0x42e01f), 0x1f);
}

TEST(NegotiateTest, getFmtpsString) {
	EXPECT_EQ(
	    getFmtpsString({"level-asymmetry-allowed=1", "packetization-mode=1",
	                    "profile-level-id=4d001f"}),
	    "level-asymmetry-allowed=1;"
	    "packetization-mode=1;"
	    "profile-level-id=4d001f");
}

TEST(NegotiateTest, getSupportedVideo) {
	auto media =
	    getSupportedMedia("0", rtc::Description::Direction::SendRecv, "video");
	EXPECT_EQ(media.generateSdp(), "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
	                               "c=IN 0.0.0.0\r\n"
	                               "a=mid:0\r\n"
	                               "a=sendrecv\r\n"
	                               "a=ssrc:1804289383 cname:0\r\n"
	                               "a=rtcp-mux\r\n"
	                               "a=rtpmap:96 H264/90000\r\n"
	                               "a=rtcp-fb:96 nack\r\n"
	                               "a=rtcp-fb:96 nack pli\r\n"
	                               "a=rtcp-fb:96 goog-remb\r\n"
	                               "a=fmtp:96 profile-level-id=42e01f;"
	                               "packetization-mode=1;"
	                               "level-asymmetry-allowed=1\r\n");
}

TEST(NegotiateTest, negotiateAnswerMedia) {
	rtc::Description remoteDesc("v=0\r\n"
	                            "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                            "s=-\r\n"
	                            "t=0 0\r\n"
	                            "m=video 9 UDP/TLS/RTP/SAVPF 96 109\r\n"
	                            "c=IN IP4 0.0.0.0\r\n"
	                            "a=mid:0\r\n"
	                            "a=recvonly\r\n"
	                            "a=rtcp-mux\r\n"
	                            "a=rtpmap:96 VP8/90000\r\n"
	                            "a=rtpmap:109 H264/90000\r\n"
	                            "a=fmtp:109 level-asymmetry-allowed=1;"
	                            "packetization-mode=1;"
	                            "profile-level-id=42e01f\r\n");
	auto media = negotiateAnswerMedia(
	    remoteDesc, 0, rtc::Description::Direction::SendOnly, "video");

	EXPECT_EQ(media->generateSdp(), "m=video 9 UDP/TLS/RTP/SAVPF 109\r\n"
	                                "c=IN 0.0.0.0\r\n"
	                                "a=mid:0\r\n"
	                                "a=sendonly\r\n"
	                                "a=ssrc:1681692777 cname:0\r\n"
	                                "a=rtcp-mux\r\n"
	                                "a=rtpmap:109 H264/90000\r\n"
	                                "a=rtcp-fb:109 nack\r\n"
	                                "a=rtcp-fb:109 nack pli\r\n"
	                                "a=rtcp-fb:109 goog-remb\r\n"
	                                "a=fmtp:109 level-asymmetry-allowed=1;"
	                                "packetization-mode=1;"
	                                "profile-level-id=42e01f\r\n");
}

TEST(NegotiateTest, negotiateRtpMap) {
	rtc::Description remoteDesc("v=0\r\n"
	                            "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                            "s=-\r\n"
	                            "t=0 0\r\n"
	                            "m=video 9 UDP/TLS/RTP/SAVPF 96 109\r\n"
	                            "c=IN IP4 0.0.0.0\r\n"
	                            "a=mid:0\r\n"
	                            "a=recvonly\r\n"
	                            "a=rtcp-mux\r\n"
	                            "a=rtpmap:96 VP8/90000\r\n"
	                            "a=rtpmap:109 H264/90000\r\n"
	                            "a=fmtp:109 level-asymmetry-allowed=1;"
	                            "packetization-mode=1;"
	                            "profile-level-id=42e01f\r\n");
	rtc::Description::Media media("m=video 9 UDP/TLS/RTP/SAVPF 109\r\n"
	                              "c=IN 0.0.0.0\r\n"
	                              "a=mid:0\r\n"
	                              "a=sendonly\r\n"
	                              "a=ssrc:1681692777 cname:0\r\n"
	                              "a=rtcp-mux\r\n"
	                              "a=rtpmap:109 H264/90000\r\n"
	                              "a=fmtp:109 level-asymmetry-allowed=1;"
	                              "packetization-mode=1;"
	                              "profile-level-id=42e01f\r\n");
	auto rtpMap = negotiateRtpMap(remoteDesc, media);

	EXPECT_EQ(rtpMap.payloadType, 109);
	EXPECT_EQ(rtpMap.format, "H264");
	EXPECT_EQ(rtpMap.clockRate, 90000);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}