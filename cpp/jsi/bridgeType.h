#pragma once

#include <WebrtcSpecJSI.h>
#include <rtc/rtc.hpp>

namespace facebook::react {

using LocalCandidate =
    NativeDatachannelLocalCandidate<std::string, std::string, std::string>;
template <>
struct Bridging<LocalCandidate>
    : NativeDatachannelLocalCandidateBridging<LocalCandidate> {};

using NativeTransceiver =
    NativeDatachannelNativeTransceiver<std::string, rtc::Description::Direction,
                                       std::string, std::string>;

template <>
struct Bridging<NativeTransceiver>
    : NativeDatachannelNativeTransceiverBridging<NativeTransceiver> {};

template <> struct Bridging<rtc::Description::Direction> {
	static rtc::Description::Direction fromJs(jsi::Runtime &rt,
	                                          const jsi::String &value) {
		try {
			auto str = value.utf8(rt);
			if (str == "sendonly")
				return rtc::Description::Direction::SendOnly;
			if (str == "recvonly")
				return rtc::Description::Direction::RecvOnly;
			if (str == "sendrecv")
				return rtc::Description::Direction::SendRecv;
			if (str == "inactive")
				return rtc::Description::Direction::Inactive;
			throw std::invalid_argument("Invalid direction");
		} catch (const std::logic_error &e) {
			throw jsi::JSError(rt, e.what());
		}
	}

	static jsi::String toJs(jsi::Runtime &rt,
	                        rtc::Description::Direction direction) {
		try {
			if (direction == rtc::Description::Direction::SendOnly)
				return bridging::toJs(rt, "sendonly");
			if (direction == rtc::Description::Direction::RecvOnly)
				return bridging::toJs(rt, "recvonly");
			if (direction == rtc::Description::Direction::SendRecv)
				return bridging::toJs(rt, "sendrecv");
			if (direction == rtc::Description::Direction::Inactive)
				return bridging::toJs(rt, "inactive");
			throw std::invalid_argument("Invalid direction");
		} catch (const std::logic_error &e) {
			throw jsi::JSError(rt, e.what());
		}
	}
};

template <> struct Bridging<rtc::Description::Type> {
	static rtc::Description::Type fromJs(jsi::Runtime &rt,
	                                     const jsi::String &value) {
		return rtc::Description::stringToType(value.utf8(rt));
	}

	static jsi::String toJs(jsi::Runtime &rt,
	                        rtc::Description::Type direction) {
		return bridging::toJs(rt, rtc::Description::typeToString(direction));
	}
};

} // namespace facebook::react
