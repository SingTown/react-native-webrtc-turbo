#pragma once

#include "bridgeType.h"
#include "ffmpeg.h"
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

	std::string addTrack(jsi::Runtime &rt, const std::string &pc,
	                     const std::string &kind,
	                     rtc::Description::Direction direction);

	std::string createOffer(jsi::Runtime &rt, const std::string &pc);
	std::string createAnswer(jsi::Runtime &rt, const std::string &pc);

	std::string getLocalDescription(jsi::Runtime &rt, const std::string &pc);
	void setLocalDescription(jsi::Runtime &rt, const std::string &pc,
	                         rtc::Description::Type type);

	std::string getRemoteDescription(jsi::Runtime &rt, const std::string &pc);
	void setRemoteDescription(jsi::Runtime &rt, const std::string &pc,
	                          const std::string &sdp,
	                          rtc::Description::Type type);

	void addRemoteCandidate(jsi::Runtime &rt, const std::string &pc,
	                        const std::string &candidate,
	                        const std::string &mid);
};
std::optional<RGBAFrame> getTrackBuffer(const std::string &tr);

} // namespace facebook::react
