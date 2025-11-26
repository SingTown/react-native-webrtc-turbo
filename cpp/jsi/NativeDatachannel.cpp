#include "NativeDatachannel.h"
#include "RTCRtpReceiver.h"
#include "framepipe.h"
#include "guid.h"
#include "log.h"
#include "negotiate.h"
#include <filesystem>
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
    jsi::Runtime &, const std::vector<std::string> &servers) {
	try {

		rtc::Configuration c;
		for (const auto &server : servers) {
			c.iceServers.emplace_back(server);
		}
		auto peerConnection = std::make_shared<rtc::PeerConnection>(c);
		std::string pc = emplacePeerConnection(peerConnection);

		peerConnection->onStateChange(
		    [this, pc](rtc::PeerConnection::State state) {
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
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

rtc::PeerConnection::GatheringState
NativeDatachannel::getGatheringState(jsi::Runtime &, const std::string &pc) {
	try {
		auto peerConnection = getPeerConnection(pc);
		return peerConnection->gatheringState();
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

rtc::PeerConnection::State
NativeDatachannel::getPeerConnectionState(jsi::Runtime &,
                                          const std::string &pc) {
	try {
		auto peerConnection = getPeerConnection(pc);
		return peerConnection->state();
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

void NativeDatachannel::closePeerConnection(jsi::Runtime &,
                                            const std::string &pc) {
	try {
		auto peerConnection = getPeerConnection(pc);
		peerConnection->close();
		peerConnectionMap.erase(pc);
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

std::string NativeDatachannel::createRTCRtpTransceiver(
    jsi::Runtime &, const std::string &pc, int index, const std::string &kind,
    rtc::Description::Direction direction, const std::string &sendPipeId,
    const std::string &recvPipeId, const std::vector<std::string> &msids,
    const std::optional<std::string> &trackid) {

	try {
		auto peerConnection = getPeerConnection(pc);
		auto track = addTransceiver(peerConnection, index, kind, direction,
		                            sendPipeId, recvPipeId, msids, trackid);

		return emplaceTrack(track);
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

void NativeDatachannel::stopRTCTransceiver(jsi::Runtime &,
                                           const std::string &tr) {
	try {
		auto track = getTrack(tr);
		if (track) {
			track->close();
			trackMap.erase(tr);
		}
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

std::string NativeDatachannel::createOffer(jsi::Runtime &,
                                           const std::string &pc) {

	try {
		auto peerConnection = getPeerConnection(pc);
		return peerConnection->createOffer();
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

std::string NativeDatachannel::createAnswer(jsi::Runtime &,
                                            const std::string &pc) {

	try {
		auto peerConnection = getPeerConnection(pc);
		return peerConnection->createAnswer();
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

std::string NativeDatachannel::getLocalDescription(jsi::Runtime &,
                                                   const std::string &pc) {
	try {
		auto peerConnection = getPeerConnection(pc);
		auto sdp = peerConnection->localDescription();
		if (!sdp.has_value()) {
			return "";
		}
		return sdp->generateSdp();
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

void NativeDatachannel::setLocalDescription(jsi::Runtime &,
                                            const std::string &pc,
                                            const std::string &sdp) {

	try {
		auto peerConnection = getPeerConnection(pc);
		rtc::Description description(sdp);
		peerConnection->setLocalDescription(description.type());
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

std::string NativeDatachannel::getRemoteDescription(jsi::Runtime &,
                                                    const std::string &pc) {
	try {
		auto peerConnection = getPeerConnection(pc);
		auto sdp = peerConnection->remoteDescription();
		if (!sdp.has_value()) {
			return "";
		}
		return sdp->generateSdp();
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

void NativeDatachannel::setRemoteDescription(jsi::Runtime &,
                                             const std::string &pc,
                                             const std::string &sdp) {

	try {
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
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

void NativeDatachannel::addRemoteCandidate(jsi::Runtime &,
                                           const std::string &pc,
                                           const std::string &candidate,
                                           const std::string &mid) {
	try {
		auto peerConnection = getPeerConnection(pc);
		rtc::Candidate cand(candidate, mid);
		peerConnection->addRemoteCandidate(cand);
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

int NativeDatachannel::forwardPipe(jsi::Runtime &,
                                   const std::string &fromPipeId,
                                   const std::string &toPipeId) {
	try {
		return subscribe(
		    {fromPipeId},
		    [toPipeId](std::string, int, std::shared_ptr<AVFrame> frame) {
			    publish(toPipeId, frame);
		    });
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

int NativeDatachannel::startRecording(jsi::Runtime &, const std::string &file,
                                      const std::string &audioPipeId,
                                      const std::string &videoPipeId) {

	try {
		if (std::filesystem::path(file).extension() != ".mp4") {
			throw std::invalid_argument("Only .mp4 format is supported c++");
		}
		AVCodecID audioCodecId = AV_CODEC_ID_NONE;
		AVCodecID videoCodecId = AV_CODEC_ID_NONE;
		std::vector<std::string> pipeIds;
		if (!audioPipeId.empty()) {
			pipeIds.push_back(audioPipeId);
			audioCodecId = AV_CODEC_ID_AAC;
		}
		if (!videoPipeId.empty()) {
			pipeIds.push_back(videoPipeId);
			videoCodecId = AV_CODEC_ID_H264;
		}

		auto muxer = std::make_shared<Muxer>(file, audioCodecId, videoCodecId);
		auto callback = [muxer, audioPipeId,
		                 videoPipeId](std::string pipeId, int,
		                              std::shared_ptr<AVFrame> frame) {
			if (pipeId == audioPipeId) {
				muxer->mux_audio(frame);
			}
			if (pipeId == videoPipeId) {
				muxer->mux_video(frame);
			}
		};

		auto cleanup = [muxer](int) { muxer->stop(); };

		return subscribe(pipeIds, callback, cleanup);
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

facebook::react::AsyncPromise<std::string>
NativeDatachannel::takePhoto(jsi::Runtime &rt, const std::string &file,
                             const std::string &pipeId) {
	FILE *f = nullptr;
	try {
		if (std::filesystem::path(file).extension() != ".png") {
			throw std::invalid_argument("Only .png format is supported");
		}
		f = fopen(file.c_str(), "wb");
		if (!f) {
			throw std::invalid_argument("Failed to open file " + file +
			                            " for writing");
		}
		auto promise =
		    std::make_shared<facebook::react::AsyncPromise<std::string>>(
		        rt, jsInvoker_);

		auto encoder = std::make_shared<Encoder>(AV_CODEC_ID_PNG);
		auto callback = [encoder, f, promise,
		                 this](std::string, int subscriptionId,
		                       std::shared_ptr<AVFrame> frame) {
			for (auto &packet : encoder->encode(frame)) {
				fwrite(packet->data, 1, packet->size, f);
			}
			fclose(f);
			::unsubscribe(subscriptionId);
			this->jsInvoker_->invokeAsync(
			    [promise](jsi::Runtime &) { promise->resolve(""); });
		};

		subscribe({pipeId}, callback, nullptr);
		return *promise;
	} catch (const std::exception &e) {
		fclose(f);
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

void NativeDatachannel::unsubscribe(jsi::Runtime &, int subscriptionId) {
	try {
		::unsubscribe(subscriptionId);
	} catch (const std::exception &e) {
		jsInvoker_->invokeAsync([&]() { throw e; });
		throw e;
	}
}

} // namespace facebook::react
