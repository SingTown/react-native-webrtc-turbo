#include "DatachannelImpl.h"
#include "rtc/rtc.h"

namespace facebook::react {

void localDescriptionCallback(int pc, const char *sdp, const char *type,
                              void *user_ptr) {
	DatachannelImpl *impl = static_cast<DatachannelImpl *>(user_ptr);
	impl->onLocalDescription(pc, sdp, type);
}

void localCandidateCallback(int pc, const char *cand, const char *mid,
                            void *user_ptr) {
	DatachannelImpl *impl = static_cast<DatachannelImpl *>(user_ptr);
	impl->onLocalCandidate(pc, cand, mid);
}

DatachannelImpl::DatachannelImpl(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeDatachannelTurboModuleCxxSpec(std::move(jsInvoker)) {}

int DatachannelImpl::createPeerConnection(jsi::Runtime &rt,
                                          std::vector<std::string> servers) {
	std::vector<const char *> iceServers;
	for (const auto &server : servers) {
		iceServers.push_back(server.c_str());
	}
	rtcConfiguration rtcConfig = {
	    .iceServers = iceServers.data(),
	    .iceServersCount = int(iceServers.size()),
	    .disableAutoNegotiation = true,
	};

	const int pc = rtcCreatePeerConnection(&rtcConfig);
	rtcSetUserPointer(pc, this);
	rtcSetLocalCandidateCallback(pc, localCandidateCallback);
	rtcSetLocalDescriptionCallback(pc, localDescriptionCallback);

	return pc;
}

void DatachannelImpl::closePeerConnection(jsi::Runtime &rt, int pc) {
	rtcClosePeerConnection(pc);
}

void DatachannelImpl::deletePeerConnection(jsi::Runtime &rt, int pc) {
	rtcDeletePeerConnection(pc);
}

void DatachannelImpl::setLocalDescription(jsi::Runtime &rt, int pc,
                                          std::string type) {
	rtcSetLocalDescription(pc, type.c_str());
}

std::string DatachannelImpl::getLocalDescription(jsi::Runtime &rt, int pc) {
	char buffer[10240];
	int size = rtcGetLocalDescription(pc, buffer, sizeof(buffer));
	return std::string(buffer, size);
}

void DatachannelImpl::setRemoteDescription(jsi::Runtime &rt, int pc,
                                           std::string sdp, std::string type) {
	rtcSetRemoteDescription(pc, sdp.c_str(), type.c_str());
}

std::string DatachannelImpl::getRemoteDescription(jsi::Runtime &rt, int pc) {
	char buffer[10240];
	int size = rtcGetRemoteDescription(pc, buffer, sizeof(buffer));
	return std::string(buffer, size);
}

int DatachannelImpl::addTrack(jsi::Runtime &rt, int pc, std::string sdp) {
	return rtcAddTrack(pc, sdp.c_str());
}

void DatachannelImpl::deleteTrack(jsi::Runtime &rt, int tr) {
	rtcDeleteTrack(tr);
}

void DatachannelImpl::addRemoteCandidate(jsi::Runtime &rt, int pc,
                                         std::string candidate,
                                         std::string mid) {
	rtcAddRemoteCandidate(pc, candidate.c_str(), mid.c_str());
}

void DatachannelImpl::onLocalDescription(int pc, std::string sdp,
                                         std::string type) {
	LocalDescription desc{pc, sdp, type};
	emitOnLocalDescription(desc);
}

void DatachannelImpl::onLocalCandidate(int pc, std::string candidate,
                                       std::string mid) {
	LocalCandidate cand{pc, candidate, mid};
	emitOnLocalCandidate(cand);
}

} // namespace facebook::react
