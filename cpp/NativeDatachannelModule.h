#pragma once

#include "bridgeType.h"
#include <WebrtcSpecJSI.h>
#include <memory>
#include <string>

namespace facebook::react {

class NativeDatachannelModule
    : public NativeDatachannelModuleCxxSpec<NativeDatachannelModule> {
  public:
	NativeDatachannelModule(std::shared_ptr<CallInvoker> jsInvoker);

	std::string createPeerConnection(jsi::Runtime &rt,
	                                 const std::vector<std::string> &servers);
	void closePeerConnection(jsi::Runtime &rt, const std::string &pc);
	void deletePeerConnection(jsi::Runtime &rt, const std::string &pc);

	std::string createMediaStreamTrack(jsi::Runtime &rt);
	void deleteMediaStreamTrack(jsi::Runtime &rt, const std::string &ms);

	std::string addTransceiver(jsi::Runtime &rt, const std::string &pc,
	                           const std::string &kind,
	                           rtc::Description::Direction direction,
	                           const std::string &sendms,
	                           const std::string &recvms,
	                           const std::string &type);

	std::string createOffer(jsi::Runtime &rt, const std::string &pc);
	std::string createAnswer(jsi::Runtime &rt, const std::string &pc);

	std::string getLocalDescription(jsi::Runtime &rt, const std::string &pc);
	void setLocalDescription(jsi::Runtime &rt, const std::string &pc,
	                         const std::string &sdp);

	std::string getRemoteDescription(jsi::Runtime &rt, const std::string &pc);
	void setRemoteDescription(jsi::Runtime &rt, const std::string &pc,
	                          const std::string &sdp);

	void addRemoteCandidate(jsi::Runtime &rt, const std::string &pc,
	                        const std::string &candidate,
	                        const std::string &mid);
};

} // namespace facebook::react
