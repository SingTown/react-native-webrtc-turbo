#include "NativeDatachannel.h"
#include "MediaStreamTrack.h"
#include "RTCRtpReceiver.h"
#include "guid.h"
#include "log.h"
#include <iostream>
#include <mutex>
#include <rtc/rtc.hpp>
#include <string>

void ffmpeg_callback(void *ptr, int level, const char *fmt, va_list vl) {
	LOGE(fmt, vl);
}

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

namespace facebook::react {

NativeDatachannel::NativeDatachannel(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeDatachannelCxxSpec(std::move(jsInvoker)) {
	av_log_set_level(AV_LOG_ERROR);
	av_log_set_callback(ffmpeg_callback);
}

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

std::string NativeDatachannel::createOffer(
    jsi::Runtime &rt, const std::string &pc,
    const std::vector<NativeTransceiver> &receivers) {

	auto peerConnection = getPeerConnection(pc);
	for (int i = 0; i < receivers.size(); i++) {
		auto &receiver = receivers[i];
		addTransceiver(peerConnection, i, receiver.kind, receiver.direction,
		               receiver.sendms, receiver.recvms);
	}
	return peerConnection->createOffer();
}

std::string NativeDatachannel::createAnswer(
    jsi::Runtime &rt, const std::string &pc,
    const std::vector<NativeTransceiver> &receivers) {

	auto peerConnection = getPeerConnection(pc);
	for (int i = 0; i < receivers.size(); i++) {
		auto &receiver = receivers[i];
		addTransceiver(peerConnection, i, receiver.kind, receiver.direction,
		               receiver.sendms, receiver.recvms);
	}
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
