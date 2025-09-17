#pragma once

#include <WebrtcSpecJSI.h>
#include <rtc/rtc.hpp>

namespace facebook::react {

struct TrackEvent {
  public:
	std::string pc;
	std::string mid;
	std::string trackId;
	std::vector<std::string> streamIds;
};

using LocalCandidate =
    NativeDatachannelLocalCandidate<std::string, std::string, std::string>;
template <>
struct Bridging<LocalCandidate>
    : NativeDatachannelLocalCandidateBridging<LocalCandidate> {};

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

template <> struct Bridging<TrackEvent> {
	static TrackEvent fromJs(jsi::Runtime &rt, const jsi::Object &value) {
		try {
			TrackEvent event;
			event.pc = value.getProperty(rt, "pc").asString(rt).utf8(rt);
			event.mid = value.getProperty(rt, "mid").asString(rt).utf8(rt);
			event.trackId =
			    value.getProperty(rt, "trackId").asString(rt).utf8(rt);
			auto streamIds =
			    value.getProperty(rt, "streamIds").asObject(rt).asArray(rt);
			for (size_t i = 0; i < streamIds.length(rt); ++i) {
				std::string streamId =
				    streamIds.getValueAtIndex(rt, i).asString(rt).utf8(rt);
				event.streamIds.push_back(streamId);
			}
			return event;
		} catch (const std::logic_error &e) {
			throw jsi::JSError(rt, e.what());
		}
	}

	static jsi::Object toJs(jsi::Runtime &rt, const TrackEvent &event) {
		try {
			jsi::Object obj(rt);
			obj.setProperty(rt, "pc", event.pc);
			obj.setProperty(rt, "mid", event.mid);
			obj.setProperty(rt, "trackId", event.trackId);
			jsi::Array arr = jsi::Array(rt, event.streamIds.size());
			for (size_t i = 0; i < event.streamIds.size(); ++i) {
				arr.setValueAtIndex(rt, i, event.streamIds[i]);
			}
			obj.setProperty(rt, "streamIds", arr);
			return obj;
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
