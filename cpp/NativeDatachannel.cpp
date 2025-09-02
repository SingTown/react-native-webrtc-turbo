#include "NativeDatachannel.h"
#include "MediaStreamTrack.h"
#include "guid.h"
#include <iostream>
#include <mutex>
#include <rtc/rtc.hpp>
#include <string>

std::mutex mutex;

std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>>
    peerConnectionMap;
std::unordered_map<std::string, std::shared_ptr<rtc::Track>> trackMap;

std::shared_ptr<rtc::PeerConnection> getPeerConnection(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = peerConnectionMap.find(id); it != peerConnectionMap.end())
		return it->second;
	else
		throw std::invalid_argument("PeerConnection ID does not exist");
}

std::string emplacePeerConnection(std::shared_ptr<rtc::PeerConnection> ptr) {
	std::lock_guard lock(mutex);
	std::string id = genUUIDV4();
	peerConnectionMap.emplace(std::make_pair(id, ptr));
	return id;
}

std::shared_ptr<rtc::Track> getTrack(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = trackMap.find(id); it != trackMap.end())
		return it->second;
	else
		throw std::invalid_argument("Track ID does not exist");
}

std::string emplaceTrack(std::shared_ptr<rtc::Track> ptr) {
	std::lock_guard lock(mutex);
	std::string id = genUUIDV4();
	trackMap.emplace(std::make_pair(id, ptr));
	return id;
}

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

namespace facebook::react {

NativeDatachannel::NativeDatachannel(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeDatachannelCxxSpec(std::move(jsInvoker)) {}

std::string NativeDatachannel::createPeerConnection(
    jsi::Runtime &rt, const std::vector<std::string> &servers) {
	rtc::Configuration c;
	for (const auto &server : servers) {
		c.iceServers.emplace_back(server);
	}
	auto peerConnection = std::make_shared<rtc::PeerConnection>(c);
	std::string pc = emplacePeerConnection(peerConnection);
	peerConnection->onLocalCandidate([this, pc](rtc::Candidate cand) {
		LocalCandidate localCandidate{
		    pc,
		    cand.candidate(),
		    cand.mid(),
		};
		emitOnLocalCandidate(localCandidate);
	});

	return pc;
}

void NativeDatachannel::closePeerConnection(jsi::Runtime &rt,
                                            const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	peerConnection->close();
}

void NativeDatachannel::deletePeerConnection(jsi::Runtime &rt,
                                             const std::string &pc) {
	peerConnectionMap.erase(pc);
}

std::string NativeDatachannel::createMediaStreamTrack(jsi::Runtime &rt) {
	auto mediaStreamTrack = std::make_shared<MediaStreamTrack>();
	return emplaceMediaStreamTrack(mediaStreamTrack);
}

void NativeDatachannel::deleteMediaStreamTrack(jsi::Runtime &rt,
                                               const std::string &id) {
	eraseMediaStreamTrack(id);
}

std::string NativeDatachannel::addTransceiver(
    jsi::Runtime &rt, const std::string &pc, const std::string &kind,
    rtc::Description::Direction direction, const std::string &sendms,
    const std::string &recvms, const std::string &type) {

	uint32_t ssrc = random() % UINT32_MAX;
	static int mid = 0;
	// mid++;
	std::string midStr = std::to_string(mid);

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

	auto peerConnection = getPeerConnection(pc);
	std::shared_ptr<rtc::Track> track;

	if (kind == "video") {
		rtc::Description::Video media(midStr, direction);
		media.addH264Codec(96);
		media.addSSRC(ssrc, midStr);
		track = peerConnection->addTrack(std::move(media));
	} else if (kind == "audio") {
		rtc::Description::Audio media(midStr, direction);
		media.addOpusCodec(111);
		media.addSSRC(ssrc, midStr);
		track = peerConnection->addTrack(std::move(media));
	} else {
		throw std::runtime_error("Unsupported transceiver kind: " + kind);
	}

	track->onOpen([peerConnection, track, recvStream]() {
		auto rtpMap = negotiateCodec(
		    peerConnection->remoteDescription().value(), track->description());

		// printf("negotiate PayloadType: %d, format: %s\n", rtpMap.payloadType,
		//        rtpMap.format.c_str());

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

		track->onFrame([decoder, recvStream](rtc::binary binary,
		                                     rtc::FrameInfo info) {
			auto packet = createAVPacket();
			if (!packet)
				throw std::runtime_error("Could not allocate AVPacket");

			if (av_new_packet(packet.get(), static_cast<int>(binary.size())) <
			    0) {
				throw std::runtime_error("Could not allocate AVPacket data");
			}
			std::memcpy(packet->data,
			            reinterpret_cast<const void *>(binary.data()),
			            binary.size());
			packet->pts = info.timestamp;
			packet->dts = info.timestamp;

			auto frames = decoder->decode(packet);
			for (auto frame : frames) {
				recvStream->push(frame);
			}
		});
	});
	return emplaceTrack(track);
}

std::string NativeDatachannel::createOffer(jsi::Runtime &rt,
                                           const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	return peerConnection->createOffer();
}

std::string NativeDatachannel::createAnswer(jsi::Runtime &rt,
                                            const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	return peerConnection->createAnswer();
}

std::string NativeDatachannel::getLocalDescription(jsi::Runtime &rt,
                                                   const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	auto sdp = peerConnection->localDescription();
	if (!sdp.has_value()) {
		return "";
	}
	return sdp->generateSdp();
}

void NativeDatachannel::setLocalDescription(jsi::Runtime &rt,
                                            const std::string &pc,
                                            const std::string &sdp) {

	auto peerConnection = getPeerConnection(pc);
	rtc::Description description(sdp);
	peerConnection->setLocalDescription(description.type());
}

std::string NativeDatachannel::getRemoteDescription(jsi::Runtime &rt,
                                                    const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	auto sdp = peerConnection->remoteDescription();
	if (!sdp.has_value()) {
		return "";
	}
	return sdp->generateSdp();
}

void NativeDatachannel::setRemoteDescription(jsi::Runtime &rt,
                                             const std::string &pc,
                                             const std::string &sdp) {

	auto peerConnection = getPeerConnection(pc);
	rtc::Description description(sdp);
	peerConnection->setRemoteDescription(description);
}

void NativeDatachannel::addRemoteCandidate(jsi::Runtime &rt,
                                           const std::string &pc,
                                           const std::string &candidate,
                                           const std::string &mid) {
	auto peerConnection = getPeerConnection(pc);
	rtc::Candidate cand(candidate, mid);
	peerConnection->addRemoteCandidate(cand);
}

} // namespace facebook::react
