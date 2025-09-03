#include "RTCRtpReceiver.h"

rtc::Description::Media::RtpMap
negotiateCodec(const rtc::Description &remoteDesc,
               const rtc::Description::Media trackMedia) {
	for (int i = 0; i < remoteDesc.mediaCount(); i++) {
		auto remoteMediaVar = remoteDesc.media(i);
		if (!std::holds_alternative<const rtc::Description::Media *>(
		        remoteMediaVar))
			continue;
		auto remoteMedia =
		    std::get<const rtc::Description::Media *>(remoteMediaVar);

		if (remoteMedia->type() != trackMedia.type())
			continue;

		for (auto remotePt : remoteMedia->payloadTypes()) {
			for (auto trackPt : trackMedia.payloadTypes()) {
				if (remotePt != trackPt)
					continue;

				return *trackMedia.rtpMap(remotePt);
			}
		}
	}

	throw std::runtime_error("No matching codec found for negotiation");
}

void SenderOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                  std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                  std::shared_ptr<rtc::Track> track) {
	const std::string cname = "singtown";
	const size_t mtu = 1200;

	auto rtpMap = negotiateCodec(peerConnection->remoteDescription().value(),
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
		    ssrc, cname, 96, rtc::H264RtpPacketizer::ClockRate);
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

	auto rtpMap = negotiateCodec(peerConnection->remoteDescription().value(),
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
		std::memcpy(packet->data, reinterpret_cast<const void *>(binary.data()),
		            binary.size());
		packet->pts = info.timestamp;
		packet->dts = info.timestamp;

		auto frames = decoder->decode(packet);
		for (auto frame : frames) {
			mediaStreamTrack->push(frame);
		}
	});
}