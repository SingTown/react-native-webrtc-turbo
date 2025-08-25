#include "NativeDatachannelModule.h"
#include "guid.h"
#include <iostream>
#include <mutex>
#include <rtc/rtc.hpp>
#include <string>

std::mutex mutex;

std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>>
    peerConnectionMap;
std::unordered_map<std::string, std::shared_ptr<rtc::Track>> trackMap;
std::unordered_map<std::string, std::shared_ptr<Decoder>> decoderMap;

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

std::shared_ptr<Decoder> getDecoder(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = decoderMap.find(id); it != decoderMap.end())
		return it->second;
	else
		return nullptr;
}

std::string emplaceDecoder(std::shared_ptr<Decoder> ptr,
                           const std::string &tr) {
	std::lock_guard lock(mutex);
	decoderMap.emplace(std::make_pair(tr, ptr));
	return tr;
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

NativeDatachannelModule::NativeDatachannelModule(
    std::shared_ptr<CallInvoker> jsInvoker)
    : NativeDatachannelModuleCxxSpec(std::move(jsInvoker)) {}

std::string NativeDatachannelModule::createPeerConnection(
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

void NativeDatachannelModule::closePeerConnection(jsi::Runtime &rt,
                                                  const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	peerConnection->close();
}

void NativeDatachannelModule::deletePeerConnection(jsi::Runtime &rt,
                                                   const std::string &pc) {
	peerConnectionMap.erase(pc);
}

std::string
NativeDatachannelModule::addTrack(jsi::Runtime &rt, const std::string &pc,
                                  const std::string &kind,
                                  rtc::Description::Direction direction) {

	uint32_t ssrc = random() % UINT32_MAX;
	static int mid = 0;
	// mid++;
	std::string midStr = std::to_string(mid);

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
	std::string tr = emplaceTrack(track);

	track->onOpen([pc, tr]() {
		auto peerConnection = getPeerConnection(pc);
		auto track = getTrack(tr);

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
		emplaceDecoder(decoder, tr);

		track->onFrame([tr](rtc::binary frame, rtc::FrameInfo info) {
			auto decoder = getDecoder(tr);
			decoder->sendPacket(frame);
		});
	});
	return tr;
}

std::string NativeDatachannelModule::createOffer(jsi::Runtime &rt,
                                                 const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	return peerConnection->createOffer();
}

std::string NativeDatachannelModule::createAnswer(jsi::Runtime &rt,
                                                  const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	return peerConnection->createAnswer();
}

std::string
NativeDatachannelModule::getLocalDescription(jsi::Runtime &rt,
                                             const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	auto sdp = peerConnection->localDescription();
	if (!sdp.has_value()) {
		return "";
	}
	return sdp->generateSdp();
}

void NativeDatachannelModule::setLocalDescription(jsi::Runtime &rt,
                                                  const std::string &pc,
                                                  rtc::Description::Type type) {

	auto peerConnection = getPeerConnection(pc);
	peerConnection->setLocalDescription(type);
}

std::string
NativeDatachannelModule::getRemoteDescription(jsi::Runtime &rt,
                                              const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	auto sdp = peerConnection->remoteDescription();
	if (!sdp.has_value()) {
		return "";
	}
	return sdp->generateSdp();
}

void NativeDatachannelModule::setRemoteDescription(
    jsi::Runtime &rt, const std::string &pc, const std::string &sdp,
    rtc::Description::Type type) {

	auto peerConnection = getPeerConnection(pc);
	rtc::Description description(sdp, type);
	peerConnection->setRemoteDescription(description);
}

void NativeDatachannelModule::addRemoteCandidate(jsi::Runtime &rt,
                                                 const std::string &pc,
                                                 const std::string &candidate,
                                                 const std::string &mid) {
	auto peerConnection = getPeerConnection(pc);
	rtc::Candidate cand(candidate, mid);
	peerConnection->addRemoteCandidate(cand);
}

std::optional<RGBAFrame> getTrackBuffer(const std::string &tr) {
	if (tr.empty()) {
		return std::nullopt;
	}
	auto decoder = getDecoder(tr);
	if (!decoder) {
		return std::nullopt;
	}
	return decoder->receiveFrame();
}

} // namespace facebook::react
