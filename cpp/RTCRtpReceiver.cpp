#include "RTCRtpReceiver.h"
#include "ffmpeg.h"
#include "framepipe.h"
#include "negotiate.h"
#include <set>

void SenderOnOpen(std::shared_ptr<rtc::Track> track, const std::string &pipeId,
                  rtc::Description::Media::RtpMap rtpMap) {
	const size_t mtu = 1200;
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
		track->chainMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_H265;
	} else if (rtpMap.format == "H264") {
		auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
		    ssrc, track->mid(), rtpMap.payloadType,
		    rtc::H264RtpPacketizer::ClockRate);
		auto packetizer =
		    std::make_shared<rtc::H264RtpPacketizer>(separator, rtpConfig, mtu);
		track->chainMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_H264;
	} else if (rtpMap.format == "opus") {
		auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
		    ssrc, track->mid(), rtpMap.payloadType,
		    rtc::OpusRtpPacketizer::DefaultClockRate);
		auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
		track->chainMediaHandler(packetizer);
		avCodecId = AV_CODEC_ID_OPUS;
	} else {
		throw std::runtime_error("Unsupported codec: " + rtpMap.format);
	}

	auto encoder = std::make_shared<Encoder>(avCodecId);
	int subscriptionId =
	    subscribe(pipeId, [encoder, track](std::shared_ptr<AVFrame> frame) {
		    if (!frame) {
			    return;
		    }
		    auto packets = encoder->encode(frame);
		    for (auto packet : packets) {
			    if (!track->isOpen()) {
				    return;
			    }
			    track->sendFrame((const rtc::byte *)packet->data, packet->size,
			                     packet->pts);
		    }
	    });
	track->onClosed([subscriptionId]() { unsubscribe(subscriptionId); });
}

void ReceiverOnOpen(std::shared_ptr<rtc::Track> track,
                    const std::string &pipeId,
                    rtc::Description::Media::RtpMap rtpMap) {
	AVCodecID avCodecId;
	auto separator = rtc::NalUnit::Separator::StartSequence;
	if (rtpMap.format == "H265") {
		auto depacketizer =
		    std::make_shared<rtc::H265RtpDepacketizer>(separator);
		track->chainMediaHandler(depacketizer);
		avCodecId = AV_CODEC_ID_H265;
	} else if (rtpMap.format == "H264") {
		auto depacketizer =
		    std::make_shared<rtc::H264RtpDepacketizer>(separator);
		track->chainMediaHandler(depacketizer);
		avCodecId = AV_CODEC_ID_H264;
	} else if (rtpMap.format == "opus") {
		auto depacketizer = std::make_shared<rtc::OpusRtpDepacketizer>();
		track->chainMediaHandler(depacketizer);
		avCodecId = AV_CODEC_ID_OPUS;
	} else {
		throw std::runtime_error("Unsupported codec: " + rtpMap.format);
	}

	auto decoder = std::make_shared<Decoder>(avCodecId);

	track->onFrame([decoder, pipeId](rtc::binary binary, rtc::FrameInfo info) {
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
			publish(pipeId, frame);
		}
	});
}

std::shared_ptr<rtc::Track>
addTransceiver(std::shared_ptr<rtc::PeerConnection> peerConnection, int index,
               const std::string &kind, rtc::Description::Direction direction,
               const std::string &sendPipeId, const std::string &recvPipeId,
               const std::vector<std::string> &msids,
               const std::optional<std::string> &trackid) {

	std::shared_ptr<rtc::Track> track;
	auto remoteDesc = peerConnection->remoteDescription();
	if (!remoteDesc) {
		auto media = getSupportedMedia(std::to_string(index), direction, kind,
		                               msids, trackid);
		track = peerConnection->addTrack(std::move(media));
	} else {
		auto media = negotiateAnswerMedia(*remoteDesc, index, direction, kind,
		                                  msids, trackid);
		if (!media) {
			throw std::runtime_error("No matching media found for index: " +
			                         std::to_string(index));
		}
		track = peerConnection->addTrack(std::move(*media));
	}

	track->onOpen([peerConnection, track, sendPipeId, recvPipeId]() {
		auto rtpMap = negotiateRtpMap(
		    peerConnection->remoteDescription().value(),
		    peerConnection->localDescription().value(), track->mid());
		if (!rtpMap) {
			return;
		}

		if (!sendPipeId.empty()) {
			SenderOnOpen(track, sendPipeId, rtpMap.value());
		}

		if (!recvPipeId.empty()) {
			ReceiverOnOpen(track, recvPipeId, rtpMap.value());
		}
	});
	return track;
}
