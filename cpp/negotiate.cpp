#include "negotiate.h"
#include <numeric>

std::optional<rtc::Description::Media>
getMedia(const rtc::Description &description, int index) {
	auto mediaVar = description.media(index);
	if (!std::holds_alternative<const rtc::Description::Media *>(mediaVar))
		return std::nullopt;
	return *std::get<const rtc::Description::Media *>(mediaVar);
}

std::string extractFmtpStringValue(const std::string &fmtp,
                                   const std::string &key) {
	auto pos = fmtp.find(key + "=");
	if (pos == std::string::npos) {
		return "";
	}

	auto start = pos + key.length() + 1;
	auto end = fmtp.find(';', start);
	if (end == std::string::npos) {
		end = fmtp.length();
	}

	return fmtp.substr(start, end - start);
}

int extractFmtpIntValue(const std::string &fmtp, const std::string &key,
                        int base) {

	auto value = extractFmtpStringValue(fmtp, key);
	if (value.empty()) {
		throw std::invalid_argument("Key not found");
	}

	return std::stoi(value, nullptr, base);
}

int getFmtpsProfileLevelId(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "profile-level-id", 16);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0x42e01f;
}

int getFmtpsPacketizationMode(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "packetization-mode", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0;
}

int getFmtpsLevelAsymmetryAllowed(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "level-asymmetry-allowed", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0;
}

int getProfileId(int profileLevelId) { return (profileLevelId >> 16) & 0xFF; }
int getConstrainedId(int profileLevelId) {
	return (profileLevelId & 0xFF00) >> 8;
}
int getLevelId(int profileLevelId) { return profileLevelId & 0xFF; }

bool isRtpMapMatchExceptPayloadType(const rtc::Description::Media::RtpMap *a,
                                    const rtc::Description::Media::RtpMap *b) {

	if (a->format != b->format) {
		return false;
	}
	if (a->format == "H264") {
		int aProfileLevelId = getFmtpsProfileLevelId(a->fmtps);
		int bProfileLevelId = getFmtpsProfileLevelId(b->fmtps);

		int aPacketizationMode = getFmtpsPacketizationMode(a->fmtps);
		int bPacketizationMode = getFmtpsPacketizationMode(b->fmtps);

		int aLevelAsymmetryAllowed = getFmtpsLevelAsymmetryAllowed(a->fmtps);
		int bLevelAsymmetryAllowed = getFmtpsLevelAsymmetryAllowed(b->fmtps);

		if (aLevelAsymmetryAllowed != bLevelAsymmetryAllowed)
			return false;
		if (aPacketizationMode != bPacketizationMode)
			return false;
		if (aLevelAsymmetryAllowed == 0) {
			if (aProfileLevelId != bProfileLevelId)
				return false;
		} else {
			if (getProfileId(aProfileLevelId) != getProfileId(bProfileLevelId))
				return false;
			if (getConstrainedId(aProfileLevelId) !=
			    getConstrainedId(bProfileLevelId))
				return false;
		}
	} else {
		return false;
	}

	return true;
}

std::string getFmtpsString(const std::vector<std::string> &fmtps) {
	return std::accumulate(fmtps.begin(), fmtps.end(), std::string{},
	                       [](const std::string &a, const std::string &b) {
		                       return a.empty() ? b : a + ";" + b;
	                       });
}

void addSupportedVideo(rtc::Description::Video &media) {
	media.addH264Codec(96, "profile-level-id=42e01f;"
	                       "packetization-mode=1;"
	                       "level-asymmetry-allowed=1");
}

void addSupportedAudio(rtc::Description::Audio &media) {
	media.addOpusCodec(111);
}

rtc::Description::Media getSupportedMedia(const std::string &mid,
                                          rtc::Description::Direction dir,
                                          const std::string &kind) {
	uint32_t ssrc = random() % UINT32_MAX;
	if (kind == "video") {
		rtc::Description::Video media(mid, dir);
		addSupportedVideo(media);
		media.addSSRC(ssrc, media.mid());
		return media;
	} else if (kind == "audio") {
		rtc::Description::Audio media(mid, dir);
		addSupportedAudio(media);
		media.addSSRC(ssrc, media.mid());
		return media;
	} else {
		throw std::invalid_argument("Unsupported media type: " + kind);
	}
}

std::optional<rtc::Description::Media>
negotiateAnswerMedia(const rtc::Description &remoteDesc, int index,
                     rtc::Description::Direction direction,
                     const std::string &kind) {
	auto remoteMedia = getMedia(remoteDesc, index);
	if (!remoteMedia)
		return std::nullopt;

	auto supportedMedia =
	    getSupportedMedia(remoteMedia->mid(), direction, kind);

	// replace payload type
	uint32_t ssrc = random() % UINT32_MAX;
	rtc::Description::Video result(remoteMedia->mid(), direction);

	for (auto supportedPt : supportedMedia.payloadTypes()) {
		auto supportRtpMap = supportedMedia.rtpMap(supportedPt);
		if (!supportRtpMap)
			continue;
		for (auto remotePt : remoteMedia->payloadTypes()) {
			auto remotRtpMap = remoteMedia->rtpMap(remotePt);
			if (!remotRtpMap) {
				continue;
			}
			std::string format = remotRtpMap->format;
			if (format != supportRtpMap->format) {
				continue;
			}
			if (isRtpMapMatchExceptPayloadType(remotRtpMap, supportRtpMap)) {
				result.addVideoCodec(remotRtpMap->payloadType, format,
				                     getFmtpsString(remotRtpMap->fmtps));
			}
		}
	}
	result.addSSRC(ssrc, remoteMedia->mid());
	return result;
}

rtc::Description::Media::RtpMap
negotiateRtpMap(const rtc::Description &remoteDesc,
                const rtc::Description::Media &trackMedia) {
	for (int i = 0; i < remoteDesc.mediaCount(); i++) {
		auto remoteMedia = getMedia(remoteDesc, i);
		if (!remoteMedia)
			continue;
		if (remoteMedia->mid() != trackMedia.mid())
			continue;
		for (auto remotePt : remoteMedia->payloadTypes()) {
			for (auto trackPt : trackMedia.payloadTypes()) {
				if (remotePt != trackPt)
					continue;
				return *trackMedia.rtpMap(trackPt);
			}
		}
	}

	throw std::runtime_error("No matching codec found for negotiation");
}