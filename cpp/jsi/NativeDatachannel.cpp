#include "NativeDatachannel.h"
#include "MediaStreamTrack.h"
#include "RTCRtpReceiver.h"
#include "guid.h"
#include "log.h"
#include "negotiate.h"
#include <iostream>
#include <mutex>
#include <regex>
#include <rtc/rtc.hpp>
#include <string>

static std::mutex mutex;

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
    : NativeDatachannelCxxSpec(std::move(jsInvoker)) {}

std::string NativeDatachannel::createPeerConnection(
    [[maybe_unused]] jsi::Runtime &rt,
    const std::vector<std::string> &servers) {
	rtc::Configuration c;
	for (const auto &server : servers) {
		c.iceServers.emplace_back(server);
	}
	auto peerConnection = std::make_shared<rtc::PeerConnection>(c);
	std::string pc = emplacePeerConnection(peerConnection);

	peerConnection->onStateChange([this, pc](rtc::PeerConnection::State state) {
		ConnectionStateChangeEvent event{
		    pc,
		    state,
		};
		emitOnConnectionStateChange(event);
	});

	peerConnection->onGatheringStateChange(
	    [this, pc](rtc::PeerConnection::GatheringState state) {
		    if (state == rtc::PeerConnection::GatheringState::Complete) {
			    LocalCandidateEvent event{
			        pc,
			        std::nullopt,
			        std::nullopt,
			    };
			    emitOnLocalCandidate(event);
		    }

		    GatheringStateChangeEvent event{pc, state};
		    emitOnGatheringStateChange(event);
	    });

	peerConnection->onLocalCandidate([this, pc](rtc::Candidate cand) {
		LocalCandidateEvent event{
		    pc,
		    cand.candidate(),
		    cand.mid(),
		};
		emitOnLocalCandidate(event);
	});

	return pc;
}

rtc::PeerConnection::GatheringState
NativeDatachannel::getGatheringState(jsi::Runtime &rt, const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	return peerConnection->gatheringState();
}

rtc::PeerConnection::State
NativeDatachannel::getPeerConnectionState(jsi::Runtime &rt,
                                          const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	return peerConnection->state();
}

void NativeDatachannel::closePeerConnection([[maybe_unused]] jsi::Runtime &rt,
                                            const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	peerConnection->close();
	peerConnectionMap.erase(pc);
}

std::string
NativeDatachannel::createMediaStreamTrack([[maybe_unused]] jsi::Runtime &rt,
                                          const std::string &kind) {
	if (kind == "audio") {
		auto mediaStreamTrack = std::make_shared<AudioStreamTrack>();
		return emplaceAudioStreamTrack(mediaStreamTrack);
	} else if (kind == "video") {
		auto mediaStreamTrack = std::make_shared<VideoStreamTrack>();
		return emplaceVideoStreamTrack(mediaStreamTrack);
	}
	throw std::invalid_argument("kind must be 'audio' or 'video'");
}

std::string NativeDatachannel::createRTCRtpTransceiver(
    [[maybe_unused]] jsi::Runtime &rt, const std::string &pc, int index,
    const std::string &kind, rtc::Description::Direction direction,
    const std::string &sendms, const std::string &recvms,
    const std::vector<std::string> &msids,
    const std::optional<std::string> &trackid) {

	auto peerConnection = getPeerConnection(pc);
	auto track = addTransceiver(peerConnection, index, kind, direction, sendms,
	                            recvms, msids, trackid);

	return emplaceTrack(track);
}

void NativeDatachannel::stopRTCTransceiver([[maybe_unused]] jsi::Runtime &rt,
                                           const std::string &tr) {
	auto track = getTrack(tr);
	if (track) {
		track->close();
		trackMap.erase(tr);
	}
}

void NativeDatachannel::stopMediaStreamTrack([[maybe_unused]] jsi::Runtime &rt,
                                             const std::string &id) {
	eraseMediaStreamTrack(id);
}

std::string NativeDatachannel::createOffer([[maybe_unused]] jsi::Runtime &rt,
                                           const std::string &pc) {

	auto peerConnection = getPeerConnection(pc);
	return peerConnection->createOffer();
}

std::string NativeDatachannel::createAnswer([[maybe_unused]] jsi::Runtime &rt,
                                            const std::string &pc) {

	auto peerConnection = getPeerConnection(pc);
	return peerConnection->createAnswer();
}

std::string
NativeDatachannel::getLocalDescription([[maybe_unused]] jsi::Runtime &rt,
                                       const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	auto sdp = peerConnection->localDescription();
	if (!sdp.has_value()) {
		return "";
	}
	return sdp->generateSdp();
}

void NativeDatachannel::setLocalDescription([[maybe_unused]] jsi::Runtime &rt,
                                            const std::string &pc,
                                            const std::string &sdp) {

	auto peerConnection = getPeerConnection(pc);
	rtc::Description description(sdp);
	peerConnection->setLocalDescription(description.type());
}

std::string
NativeDatachannel::getRemoteDescription([[maybe_unused]] jsi::Runtime &rt,
                                        const std::string &pc) {
	auto peerConnection = getPeerConnection(pc);
	auto sdp = peerConnection->remoteDescription();
	if (!sdp.has_value()) {
		return "";
	}
	return sdp->generateSdp();
}

void NativeDatachannel::setRemoteDescription([[maybe_unused]] jsi::Runtime &rt,
                                             const std::string &pc,
                                             const std::string &sdp) {

	auto peerConnection = getPeerConnection(pc);
	rtc::Description description(sdp);
	peerConnection->setRemoteDescription(description);
	for (int i = 0; i < description.mediaCount(); i++) {
		auto media = getMediaFromIndex(description, i);
		if (!media) {
			continue;
		}
		if (media->direction() == rtc::Description::Direction::Inactive ||
		    media->direction() == rtc::Description::Direction::RecvOnly) {
			continue;
		}
		TrackEvent result;
		result.pc = pc;
		result.mid = media->mid();
		result.trackId = "";
		result.streamIds = {};
		for (std::string &attr : media->attributes()) {
			std::regex re(R"(msid:([^\s]+)\s+([^\s]+))");
			std::smatch match;
			if (std::regex_match(attr, match, re) && match.size() == 3) {
				result.trackId = match[2];
				result.streamIds.push_back(match[1]);
			}
		}
		emitOnTrack(result);
	}
}

void NativeDatachannel::addRemoteCandidate([[maybe_unused]] jsi::Runtime &rt,
                                           const std::string &pc,
                                           const std::string &candidate,
                                           const std::string &mid) {
	auto peerConnection = getPeerConnection(pc);
	rtc::Candidate cand(candidate, mid);
	peerConnection->addRemoteCandidate(cand);
}

} // namespace facebook::react
