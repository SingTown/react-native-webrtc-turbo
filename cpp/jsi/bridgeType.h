#pragma once

#include <WebrtcSpecJSI.h>
#include <rtc/rtc.hpp>

namespace facebook::react {

using TrackEvent =
    NativeDatachannelTrackEvent<std::string, std::string, std::string,
                                std::vector<std::string>>;
template <>
struct Bridging<TrackEvent> : NativeDatachannelTrackEventBridging<TrackEvent> {
};

using ConnectionStateChangeEvent =
    NativeDatachannelConnectionStateChangeEvent<std::string,
                                                rtc::PeerConnection::State>;
template <>
struct Bridging<ConnectionStateChangeEvent>
    : NativeDatachannelConnectionStateChangeEventBridging<
          ConnectionStateChangeEvent> {};

using GatheringStateChangeEvent = NativeDatachannelGatheringStateChangeEvent<
    std::string, rtc::PeerConnection::GatheringState>;
template <>
struct Bridging<GatheringStateChangeEvent>
    : NativeDatachannelGatheringStateChangeEventBridging<
          GatheringStateChangeEvent> {};

using LocalCandidateEvent = NativeDatachannelLocalCandidateEvent<
    std::string, std::optional<std::string>, std::optional<std::string>>;
template <>
struct Bridging<LocalCandidateEvent>
    : NativeDatachannelLocalCandidateEventBridging<LocalCandidateEvent> {};

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

template <> struct Bridging<rtc::PeerConnection::GatheringState> {
	static rtc::PeerConnection::GatheringState
	fromJs(jsi::Runtime &rt, const jsi::String &value) {
		if (value.utf8(rt) == "new")
			return rtc::PeerConnection::GatheringState::New;
		else if (value.utf8(rt) == "gathering")
			return rtc::PeerConnection::GatheringState::InProgress;
		else if (value.utf8(rt) == "complete")
			return rtc::PeerConnection::GatheringState::Complete;
		else
			throw jsi::JSError(rt, "Invalid gathering state");
	}

	static jsi::String toJs(jsi::Runtime &rt,
	                        rtc::PeerConnection::GatheringState state) {
		if (state == rtc::PeerConnection::GatheringState::New)
			return bridging::toJs(rt, "new");
		else if (state == rtc::PeerConnection::GatheringState::InProgress)
			return bridging::toJs(rt, "gathering");
		else if (state == rtc::PeerConnection::GatheringState::Complete)
			return bridging::toJs(rt, "complete");
		else
			throw jsi::JSError(rt, "Invalid gathering state");
	}
};

template <> struct Bridging<rtc::PeerConnection::State> {
	static rtc::PeerConnection::State fromJs(jsi::Runtime &rt,
	                                         const jsi::String &value) {
		if (value.utf8(rt) == "new")
			return rtc::PeerConnection::State::New;
		else if (value.utf8(rt) == "connecting")
			return rtc::PeerConnection::State::Connecting;
		else if (value.utf8(rt) == "connected")
			return rtc::PeerConnection::State::Connected;
		else if (value.utf8(rt) == "disconnected")
			return rtc::PeerConnection::State::Disconnected;
		else if (value.utf8(rt) == "failed")
			return rtc::PeerConnection::State::Failed;
		else if (value.utf8(rt) == "closed")
			return rtc::PeerConnection::State::Closed;
		else
			throw jsi::JSError(rt, "Invalid peerconnection state");
	}

	static jsi::String toJs(jsi::Runtime &rt,
	                        rtc::PeerConnection::State state) {
		if (state == rtc::PeerConnection::State::New)
			return bridging::toJs(rt, "new");
		else if (state == rtc::PeerConnection::State::Connecting)
			return bridging::toJs(rt, "connecting");
		else if (state == rtc::PeerConnection::State::Connected)
			return bridging::toJs(rt, "connected");
		else if (state == rtc::PeerConnection::State::Disconnected)
			return bridging::toJs(rt, "disconnected");
		else if (state == rtc::PeerConnection::State::Failed)
			return bridging::toJs(rt, "failed");
		else if (state == rtc::PeerConnection::State::Closed)
			return bridging::toJs(rt, "closed");
		else
			throw jsi::JSError(rt, "Invalid peerconnection state");
	}
};

} // namespace facebook::react
