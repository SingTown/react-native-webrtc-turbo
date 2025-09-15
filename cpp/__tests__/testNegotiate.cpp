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

TEST(NegotiateTest, H264Property) {
	std::vector<std::string> a = {"level-asymmetry-allowed=1",
	                              "packetization-mode=1",
	                              "profile-level-id=4d001f"};
	std::vector<std::string> b = {"level-asymmetry-allowed=0",
	                              "packetization-mode=0",
	                              "profile-level-id=0x42e01f"};
	EXPECT_EQ(getH264ProfileId(a), 0x4d);
	EXPECT_EQ(getH264ProfileId(b), 0x42);

	EXPECT_EQ(getH264ConstrainedId(a), 0x00);
	EXPECT_EQ(getH264ConstrainedId(b), 0xe0);

	EXPECT_EQ(getH264PacketizationMode(a), 1);
	EXPECT_EQ(getH264PacketizationMode(b), 0);

	EXPECT_EQ(getH264LevelAsymmetryAllowed(a), 1);
	EXPECT_EQ(getH264LevelAsymmetryAllowed(b), 0);

	EXPECT_EQ(getH264LevelId(a), 0x1f);
	EXPECT_EQ(getH264LevelId(b), 0x1f);
}

TEST(NegotiateTest, H265Property) {
	std::vector<std::string> a = {"level-id=96", "tx-mode=SRST"};
	std::vector<std::string> b = {"level-id=180", "profile-id=2", "tier-flag=0",
	                              "tx-mode=SRST"};

	EXPECT_EQ(getH265ProfileId(a), 1);
	EXPECT_EQ(getH265ProfileId(b), 2);

	EXPECT_EQ(getH265TierFlag(a), 0);
	EXPECT_EQ(getH265TierFlag(b), 0);

	EXPECT_EQ(getH265LevelId(a), 96);
	EXPECT_EQ(getH265LevelId(b), 180);
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
	auto sdp = media.generateSdp();
	EXPECT_TRUE(sdp.find("m=video 9 UDP/TLS/RTP/SAVPF 104 96") !=
	            std::string::npos);
	EXPECT_TRUE(sdp.find("a=mid:0") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=sendrecv") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:96 H264/90000") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:104 H265/90000") != std::string::npos);
}

TEST(NegotiateTest, getSupportedAudio) {
	auto media =
	    getSupportedMedia("0", rtc::Description::Direction::SendRecv, "audio");
	auto sdp = media.generateSdp();
	EXPECT_TRUE(sdp.find("m=audio 9 UDP/TLS/RTP/SAVPF 111") !=
	            std::string::npos);
	EXPECT_TRUE(sdp.find("a=mid:0") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=sendrecv") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:111 opus/48000/2") != std::string::npos);
}

TEST(NegotiateTest, answerH264) {
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

	auto sdp = media->generateSdp();

	EXPECT_TRUE(sdp.find("m=video 9 UDP/TLS/RTP/SAVPF 109") !=
	            std::string::npos);
	EXPECT_TRUE(sdp.find("a=mid:0") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=sendonly") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:109 H264/90000") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=fmtp:109 level-asymmetry-allowed=1;"
	                     "packetization-mode=1;"
	                     "profile-level-id=42e01f") != std::string::npos);
}

TEST(NegotiateTest, answerH265) {
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
	                            "a=rtpmap:109 H265/90000\r\n"
	                            "a=fmtp:109 level-id=93;"
	                            "profile-id=1;"
	                            "tier-flag=0;tx-mode=SRST\r\n");
	auto media = negotiateAnswerMedia(
	    remoteDesc, 0, rtc::Description::Direction::SendOnly, "video");

	auto sdp = media->generateSdp();
	EXPECT_TRUE(sdp.find("m=video 9 UDP/TLS/RTP/SAVPF 109") !=
	            std::string::npos);
	EXPECT_TRUE(sdp.find("a=mid:0") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=sendonly") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:109 H265/90000") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=fmtp:109 level-id=93;"
	                     "profile-id=1;"
	                     "tier-flag=0;tx-mode=SRST") != std::string::npos);
}

TEST(NegotiateTest, answerH265H264) {
	rtc::Description remoteDesc("v=0\r\n"
	                            "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                            "s=-\r\n"
	                            "t=0 0\r\n"
	                            "m=video 9 UDP/TLS/RTP/SAVPF 96 109\r\n"
	                            "c=IN IP4 0.0.0.0\r\n"
	                            "a=mid:0\r\n"
	                            "a=recvonly\r\n"
	                            "a=rtcp-mux\r\n"

	                            "a=rtpmap:96 H264/90000\r\n"
	                            "a=fmtp:96 level-asymmetry-allowed=1;"
	                            "packetization-mode=1;"
	                            "profile-level-id=42e01f\r\n"

	                            "a=rtpmap:109 H265/90000\r\n"
	                            "a=fmtp:109 level-id=93;"
	                            "profile-id=1;"
	                            "tier-flag=0;tx-mode=SRST\r\n");
	auto media = negotiateAnswerMedia(
	    remoteDesc, 0, rtc::Description::Direction::SendOnly, "video");
	auto sdp = media->generateSdp();
	EXPECT_TRUE(sdp.find("m=video 9 UDP/TLS/RTP/SAVPF 96 109") !=
	            std::string::npos);
	EXPECT_TRUE(sdp.find("a=mid:0") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=sendonly") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:96 H264/90000") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=fmtp:96 level-asymmetry-allowed=1;"
	                     "packetization-mode=1;"
	                     "profile-level-id=42e01f") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:109 H265/90000") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=fmtp:109 level-id=93;"
	                     "profile-id=1;"
	                     "tier-flag=0;tx-mode=SRST") != std::string::npos);
}

TEST(NegotiateTest, answerOpus) {
	rtc::Description remoteDesc("v=0\r\n"
	                            "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                            "s=-\r\n"
	                            "t=0 0\r\n"
	                            "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
	                            "c=IN IP4 0.0.0.0\r\n"
	                            "a=mid:0\r\n"
	                            "a=recvonly\r\n"
	                            "a=rtcp-mux\r\n"
	                            "a=rtpmap:111 opus/48000/2\r\n"
	                            "a=fmtp:111\r\n");
	auto media = negotiateAnswerMedia(
	    remoteDesc, 0, rtc::Description::Direction::SendOnly, "audio");

	auto sdp = media->generateSdp();
	EXPECT_TRUE(sdp.find("m=audio 9 UDP/TLS/RTP/SAVPF 111") !=
	            std::string::npos);
	EXPECT_TRUE(sdp.find("a=mid:0") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=sendonly") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=rtpmap:111 opus/48000/2") != std::string::npos);
	EXPECT_TRUE(sdp.find("a=fmtp:111") != std::string::npos);
}

TEST(NegotiateTest, negotiateRtpMap) {
	rtc::Description remoteDesc("v=0\r\n"
	                            "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                            "s=-\r\n"
	                            "t=0 0\r\n"
	                            "m=video 9 UDP/TLS/RTP/SAVPF 109\r\n"
	                            "c=IN IP4 0.0.0.0\r\n"
	                            "a=mid:0\r\n"
	                            "a=recvonly\r\n"
	                            "a=rtcp-mux\r\n"
	                            "a=rtpmap:109 H264/90000\r\n"
	                            "a=fmtp:109 level-asymmetry-allowed=1;"
	                            "packetization-mode=1;"
	                            "profile-level-id=42e01f\r\n",
	                            "offer");

	rtc::Description localDesc("v=0\r\n"
	                           "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                           "s=-\r\n"
	                           "t=0 0\r\n"
	                           "m=video 9 UDP/TLS/RTP/SAVPF 109\r\n"
	                           "c=IN IP4 0.0.0.0\r\n"
	                           "a=mid:0\r\n"
	                           "a=recvonly\r\n"
	                           "a=rtcp-mux\r\n"
	                           "a=rtpmap:109 H264/90000\r\n"
	                           "a=fmtp:109 level-asymmetry-allowed=1;"
	                           "packetization-mode=1;"
	                           "profile-level-id=42e01f\r\n",
	                           "answer");

	auto rtpMap = negotiateRtpMap(remoteDesc, localDesc, "0");

	EXPECT_EQ(rtpMap.payloadType, 109);
	EXPECT_EQ(rtpMap.format, "H264");
	EXPECT_EQ(rtpMap.clockRate, 90000);
}

TEST(NegotiateTest, negotiateRtpMapPriority) {
	rtc::Description remoteDesc("v=0\r\n"
	                            "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                            "s=-\r\n"
	                            "t=0 0\r\n"
	                            "m=video 9 UDP/TLS/RTP/SAVPF 109 96\r\n"
	                            "c=IN IP4 0.0.0.0\r\n"
	                            "a=mid:0\r\n"
	                            "a=recvonly\r\n"
	                            "a=rtcp-mux\r\n"
	                            "a=rtpmap:96 H264/90000\r\n"
	                            "a=rtpmap:109 H265/90000\r\n",
	                            rtc::Description::Type::Offer);

	rtc::Description localDesc("v=0\r\n"
	                           "o=- 0 0 IN IP4 127.0.0.1\r\n"
	                           "s=-\r\n"
	                           "t=0 0\r\n"
	                           "m=video 9 UDP/TLS/RTP/SAVPF 96 109\r\n"
	                           "c=IN IP4 0.0.0.0\r\n"
	                           "a=mid:0\r\n"
	                           "a=recvonly\r\n"
	                           "a=rtcp-mux\r\n"
	                           "a=rtpmap:96 H264/90000\r\n"
	                           "a=rtpmap:109 H265/90000\r\n",
	                           rtc::Description::Type::Answer);

	auto rtpMap = negotiateRtpMap(remoteDesc, localDesc, "0");

	EXPECT_EQ(rtpMap.payloadType, 109);
	EXPECT_EQ(rtpMap.format, "H265");
	EXPECT_EQ(rtpMap.clockRate, 90000);
}