#include "RTCRtpReceiver.h"
#include "negotiate.h"
#include <set>

void SenderOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                  std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                  std::shared_ptr<rtc::Track> track) {
	const size_t mtu = 1200;

	auto rtpMap = negotiateRtpMap(peerConnection->remoteDescription().value(),
	                              track->description());

	auto ssrcs = track->description().getSSRCs();
	if (ssrcs.size() != 1) {
		throw std::runtime_error("Expected exactly one SSRC");
	}
	rtc::SSRC ssrc = ssrcs[0];

	AVCodecID avCodecId;
	auto separator = rtc::NalUnit::Separator::StartSequence;
	// if (rtpMap.format == "H265") {
	// 	auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
	// 	    ssrc, cname, 96, rtc::H265RtpPacketizer::ClockRate);
	// 	auto packetizer = std::make_shared<rtc::H265RtpPacketizer>(
	// 	    separator, rtpConfig, mtu);
	// 	track->setMediaHandler(packetizer);
	// 	codecName = "hevc_videotoolbox";
	// } else
	if (rtpMap.format == "H264") {
		auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
		    ssrc, track->mid(), rtpMap.payloadType,
		    rtc::H264RtpPacketizer::ClockRate);
		auto packetizer =
		    std::make_shared<rtc::H264RtpPacketizer>(separator, rtpConfig, mtu);
		track->setMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_H264;
	} else {
		throw std::runtime_error("Unsupported codec: " + rtpMap.format);
	}

	auto encoder = std::make_shared<Encoder>(avCodecId);
	mediaStreamTrack->onPush(
	    [mediaStreamTrack, encoder, track](std::shared_ptr<AVFrame> frame) {
		    while (1) {
			    auto frame = mediaStreamTrack->pop(AV_PIX_FMT_NV12);
			    if (!frame) {
				    return;
			    }
			    auto packets = encoder->encode(frame);
			    for (auto packet : packets) {
				    // for (int i = 0; i < 20; i++) {
				    //     printf(" %02x", packet->data[i]);
				    // }

				    track->sendFrame((const rtc::byte *)packet->data,
				                     packet->size, packet->pts);
			    }
		    }
	    });
}

void ReceiverOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                    std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                    std::shared_ptr<rtc::Track> track) {

	auto rtpMap = negotiateRtpMap(peerConnection->remoteDescription().value(),
	                              track->description());

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
	} else {
		throw std::runtime_error("Unsupported codec: " + rtpMap.format);
	}

	auto decoder = std::make_shared<Decoder>(avCodecId);

	track->onFrame([decoder, mediaStreamTrack](rtc::binary binary,
	                                           rtc::FrameInfo info) {
		std::shared_ptr<AVPacket> packet(
		    av_packet_alloc(), [](AVPacket *f) { av_packet_free(&f); });
		if (!packet)
			throw std::runtime_error("Could not allocate AVPacket");

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

std::shared_ptr<rtc::Track>
addTransceiver(std::shared_ptr<rtc::PeerConnection> peerConnection, int index,
               const std::string &kind, rtc::Description::Direction direction,
               const std::string &sendms, const std::string &recvms) {

	std::shared_ptr<MediaStreamTrack> sendStream;
	std::shared_ptr<MediaStreamTrack> recvStream;
	if (direction == rtc::Description::Direction::SendRecv) {
		sendStream = getMediaStreamTrack(sendms);
		recvStream = getMediaStreamTrack(recvms);
	} else if (direction == rtc::Description::Direction::SendOnly) {
		sendStream = getMediaStreamTrack(sendms);
	} else if (direction == rtc::Description::Direction::RecvOnly) {
		recvStream = getMediaStreamTrack(recvms);
	}

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