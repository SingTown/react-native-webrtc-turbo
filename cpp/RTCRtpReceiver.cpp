#include "RTCRtpReceiver.h"
#include "ffmpeg.h"
#include "negotiate.h"
#include <set>

void SenderOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                  std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                  std::shared_ptr<rtc::Track> track) {
	const size_t mtu = 1200;

	auto rtpMap = negotiateRtpMap(peerConnection->remoteDescription().value(),
	                              peerConnection->localDescription().value(),
	                              track->mid());

	auto ssrcs = track->description().getSSRCs();
	if (ssrcs.size() != 1) {
		throw std::runtime_error("Expected exactly one SSRC");
	}
	rtc::SSRC ssrc = ssrcs[0];

	AVCodecID avCodecId;
	auto separator = rtc::NalUnit::Separator::StartSequence;
	if (rtpMap.format == "H265") {
		auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
		    ssrc, track->mid(), rtpMap.payloadType,
		    rtc::H265RtpPacketizer::ClockRate);
		auto packetizer =
		    std::make_shared<rtc::H265RtpPacketizer>(separator, rtpConfig, mtu);
		track->setMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_H265;
	} else if (rtpMap.format == "H264") {
		auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
		    ssrc, track->mid(), rtpMap.payloadType,
		    rtc::H264RtpPacketizer::ClockRate);
		auto packetizer =
		    std::make_shared<rtc::H264RtpPacketizer>(separator, rtpConfig, mtu);
		track->setMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_H264;
	} else if (rtpMap.format == "opus") {
		auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
		    ssrc, track->mid(), rtpMap.payloadType,
		    rtc::OpusRtpPacketizer::DefaultClockRate);
		auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
		track->setMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_OPUS;
	} else {
		throw std::runtime_error("Unsupported codec: " + rtpMap.format);
	}

	auto encoder = std::make_shared<Encoder>(avCodecId);
	mediaStreamTrack->onPush(
	    [mediaStreamTrack, encoder, track,
	     rtpMap]([[maybe_unused]] std::shared_ptr<AVFrame> frame) {
		    while (1) {
			    auto frame = mediaStreamTrack->pop();
			    if (!frame) {
				    return;
			    }
			    auto packets = encoder->encode(frame);
			    for (auto packet : packets) {
				    // for (int i = 0; i < 20; i++) {
				    //     printf(" %02x", packet->data[i]);
				    // }
				    if (track->isClosed()) {
					    return;
				    }
				    track->sendFrame((const rtc::byte *)packet->data,
				                     packet->size, packet->pts);
			    }
		    }
	    });
}

void SenderOnClose(
    [[maybe_unused]] std::shared_ptr<rtc::PeerConnection> peerConnection,
    [[maybe_unused]] std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
    [[maybe_unused]] std::shared_ptr<rtc::Track> track) {

	mediaStreamTrack->onPush(nullptr);
}

void ReceiverOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                    std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                    std::shared_ptr<rtc::Track> track) {

	auto rtpMap = negotiateRtpMap(peerConnection->remoteDescription().value(),
	                              peerConnection->localDescription().value(),
	                              track->mid());

	AVCodecID avCodecId;
	auto separator = rtc::NalUnit::Separator::StartSequence;
	if (rtpMap.format == "H265") {
		auto depacketizer =
		    std::make_shared<rtc::H265RtpDepacketizer>(separator);
		track->setMediaHandler(depacketizer);
		avCodecId = AV_CODEC_ID_H265;
	} else if (rtpMap.format == "H264") {
		auto depacketizer =
		    std::make_shared<rtc::H264RtpDepacketizer>(separator);
		track->setMediaHandler(depacketizer);
		avCodecId = AV_CODEC_ID_H264;
	} else if (rtpMap.format == "opus") {
		auto depacketizer = std::make_shared<rtc::OpusRtpDepacketizer>();
		track->setMediaHandler(depacketizer);
		avCodecId = AV_CODEC_ID_OPUS;
	} else {
		throw std::runtime_error("Unsupported codec: " + rtpMap.format);
	}

	auto decoder = std::make_shared<Decoder>(avCodecId);

	track->onFrame([decoder, mediaStreamTrack](rtc::binary binary,
	                                           rtc::FrameInfo info) {
		auto packet = createAVPacket();

		if (av_new_packet(packet.get(), static_cast<int>(binary.size())) < 0) {
			throw std::runtime_error("Could not allocate AVPacket data");
		}
		memcpy(packet->data, reinterpret_cast<const void *>(binary.data()),
		       binary.size());
		packet->pts = info.timestamp;
		packet->dts = info.timestamp;

		auto frames = decoder->decode(packet);
		for (auto frame : frames) {
			mediaStreamTrack->push(frame);
		}
	});
}

void ReceiverOnClose(
    [[maybe_unused]] std::shared_ptr<rtc::PeerConnection> peerConnection,
    [[maybe_unused]] std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
    [[maybe_unused]] std::shared_ptr<rtc::Track> track) {}

std::shared_ptr<rtc::Track>
addTransceiver(std::shared_ptr<rtc::PeerConnection> peerConnection, int index,
               const std::string &kind, rtc::Description::Direction direction,
               const std::string &sendms, const std::string &recvms) {

	auto sendStream = getMediaStreamTrack(sendms);
	auto recvStream = getMediaStreamTrack(recvms);

	std::shared_ptr<rtc::Track> track;
	auto remoteDesc = peerConnection->remoteDescription();
	if (!remoteDesc) {
		auto media = getSupportedMedia(std::to_string(index), direction, kind);
		track = peerConnection->addTrack(std::move(media));
	} else {
		auto media = negotiateAnswerMedia(*remoteDesc, index, direction, kind);
		if (!media) {
			throw std::runtime_error("No matching media found for index: " +
			                         std::to_string(index));
		}
		track = peerConnection->addTrack(std::move(*media));
	}

	track->onClosed([peerConnection, track, sendStream, recvStream]() {
		if (sendStream) {
			SenderOnClose(peerConnection, sendStream, track);
		}

		if (recvStream) {
			ReceiverOnClose(peerConnection, recvStream, track);
		}
	});

	track->onOpen([peerConnection, track, sendStream, recvStream]() {
		if (sendStream) {
			SenderOnOpen(peerConnection, sendStream, track);
		}

		if (recvStream) {
			ReceiverOnOpen(peerConnection, recvStream, track);
		}
	});
	return track;
}
