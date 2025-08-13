#include "DatachannelImpl.h"
#include "rtc/rtc.h"

namespace facebook::react {

std::string base64_encode(const char *data, size_t len) {

	const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	                            "abcdefghijklmnopqrstuvwxyz"
	                            "0123456789+/";

	std::string result;
	result.reserve(((len + 2) / 3) * 4);

	unsigned char input[3];
	unsigned char output[4];
	size_t i = 0;

	while (len--) {
		input[i++] = *(data++);
		if (i == 3) {
			output[0] = (input[0] & 0xfc) >> 2;
			output[1] = ((input[0] & 0x03) << 4) + ((input[1] & 0xf0) >> 4);
			output[2] = ((input[1] & 0x0f) << 2) + ((input[2] & 0xc0) >> 6);
			output[3] = input[2] & 0x3f;

			for (i = 0; i < 4; ++i)
				result += kBase64Chars[output[i]];
			i = 0;
		}
	}

	if (i) {
		for (size_t j = i; j < 3; ++j)
			input[j] = '\0';

		output[0] = (input[0] & 0xfc) >> 2;
		output[1] = ((input[0] & 0x03) << 4) + ((input[1] & 0xf0) >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + ((input[2] & 0xc0) >> 6);
		output[3] = input[2] & 0x3f;

		for (size_t j = 0; j < i + 1; ++j)
			result += kBase64Chars[output[j]];

		while ((i++ < 3))
			result += '=';
	}

	return result;
}

void localCandidateCallback(int pc, const char *cand, const char *mid,
                            void *user_ptr) {
	DatachannelImpl *impl = static_cast<DatachannelImpl *>(user_ptr);
	impl->onLocalCandidate(pc, cand, mid);
}

void messageCallback(int id, const char *message, int size, void *user_ptr) {
	DatachannelImpl *impl = static_cast<DatachannelImpl *>(user_ptr);
	std::string base64 = base64_encode(message, size);
	impl->onMessage(id, base64);
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

	return pc;
}

void DatachannelImpl::closePeerConnection(jsi::Runtime &rt, int pc) {
	rtcClosePeerConnection(pc);
}

void DatachannelImpl::deletePeerConnection(jsi::Runtime &rt, int pc) {
	rtcDeletePeerConnection(pc);
}

std::string DatachannelImpl::createOffer(jsi::Runtime &rt, int pc) {
	char buffer[10240];
	int size = rtcCreateOffer(pc, buffer, sizeof(buffer));
	return std::string(buffer, size);
}

std::string DatachannelImpl::createAnswer(jsi::Runtime &rt, int pc) {
	char buffer[10240];
	int size = rtcCreateAnswer(pc, buffer, sizeof(buffer));
	return std::string(buffer, size);
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
	int tr = rtcAddTrack(pc, sdp.c_str());
	rtcSetMessageCallback(tr, messageCallback);
	return tr;
}

void DatachannelImpl::deleteTrack(jsi::Runtime &rt, int tr) {
	rtcDeleteTrack(tr);
}

void DatachannelImpl::addRemoteCandidate(jsi::Runtime &rt, int pc,
                                         std::string candidate,
                                         std::string mid) {
	rtcAddRemoteCandidate(pc, candidate.c_str(), mid.c_str());
}

void DatachannelImpl::onLocalCandidate(int pc, std::string candidate,
                                       std::string mid) {
	LocalCandidate cand{pc, candidate, mid};
	emitOnLocalCandidate(cand);
}

void DatachannelImpl::onMessage(int id, std::string data) {
	Message msg{id, data};
	emitOnMessage(msg);
}
} // namespace facebook::react
