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

int getH264ProfileLevelId(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "profile-level-id", 16);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0x42e01f;
}

int getH264PacketizationMode(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "packetization-mode", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0;
}

int getH264LevelAsymmetryAllowed(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "level-asymmetry-allowed", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0;
}

int getH264ProfileId(const std::vector<std::string> &fmtps) {
	int profileLevelId = getH264ProfileLevelId(fmtps);
	return (profileLevelId >> 16) & 0xFF;
}
int getH264ConstrainedId(const std::vector<std::string> &fmtps) {
	int profileLevelId = getH264ProfileLevelId(fmtps);
	return (profileLevelId & 0xFF00) >> 8;
}
int getH264LevelId(const std::vector<std::string> &fmtps) {
	int profileLevelId = getH264ProfileLevelId(fmtps);
	return profileLevelId & 0xFF;
}

int getH265ProfileId(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "profile-id", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 1;
}

int getH265LevelId(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "level-id", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 1;
}

int getH265TierFlag(const std::vector<std::string> &fmtps) {
	for (const auto &fmtp : fmtps) {
		try {
			return extractFmtpIntValue(fmtp, "tier-flag", 10);
		} catch (const std::exception &) {
			continue;
		}
	}
	return 0;
}

bool isRtpMapMatchExceptPayloadType(const rtc::Description::Media::RtpMap *a,
                                    const rtc::Description::Media::RtpMap *b) {

	if (a->format != b->format) {
		return false;
	}
	if (a->format == "H264") {
		int aProfileId = getH264ProfileId(a->fmtps);
		int bProfileId = getH264ProfileId(b->fmtps);
		if (aProfileId != bProfileId)
			return false;

		int aPacketizationMode = getH264PacketizationMode(a->fmtps);
		int bPacketizationMode = getH264PacketizationMode(b->fmtps);
		if (aPacketizationMode != bPacketizationMode)
			return false;

		int aLevelAsymmetryAllowed = getH264LevelAsymmetryAllowed(a->fmtps);
		int bLevelAsymmetryAllowed = getH264LevelAsymmetryAllowed(b->fmtps);
		if (aLevelAsymmetryAllowed != bLevelAsymmetryAllowed)
			return false;

		if (aLevelAsymmetryAllowed == 0) {
			int aLevelId = getH264LevelId(a->fmtps);
			int bLevelId = getH264LevelId(b->fmtps);
			if (aLevelId != bLevelId)
				return false;
		}
	} else if (a->format == "H265") {
		int aProfileId = getH265ProfileId(a->fmtps);
		int bProfileId = getH265ProfileId(b->fmtps);
		if (aProfileId != bProfileId)
			return false;

		int aTierFlag = getH265TierFlag(a->fmtps);
		int bTierFlag = getH265TierFlag(b->fmtps);
		if (aTierFlag != bTierFlag)
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
	media.addH265Codec(104, "level-id=93;"
	                        "profile-id=1;"
	                        "tier-flag=0;"
	                        "tx-mode=SRST");
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
negotiateAnswerMedia(const rtc::Description &offer, int index,
                     rtc::Description::Direction direction,
                     const std::string &kind) {
	auto offerMedia = getMedia(offer, index);
	if (!offerMedia)
		return std::nullopt;

	auto supportedMedia = getSupportedMedia(offerMedia->mid(), direction, kind);

	uint32_t ssrc = random() % UINT32_MAX;
	rtc::Description::Video result(offerMedia->mid(), direction);

	for (auto offerPt : offerMedia->payloadTypes()) {
		auto offerRtpMap = offerMedia->rtpMap(offerPt);
		if (!offerRtpMap) {
			continue;
		}

		for (auto supportedPt : supportedMedia.payloadTypes()) {
			auto supportRtpMap = supportedMedia.rtpMap(supportedPt);
			if (!supportRtpMap)
				continue;

			std::string format = offerRtpMap->format;
			if (format != supportRtpMap->format) {
				continue;
			}
			if (isRtpMapMatchExceptPayloadType(offerRtpMap, supportRtpMap)) {
				result.addVideoCodec(offerRtpMap->payloadType, format,
				                     getFmtpsString(offerRtpMap->fmtps));
			}
		}
	}
	result.addSSRC(ssrc, offerMedia->mid());
	return result;
}

rtc::Description::Media::RtpMap
negotiateRtpMap(const rtc::Description &remoteDesc,
                const rtc::Description &localDesc, const std::string &mid) {

	auto offer = remoteDesc.type() == rtc::Description::Type::Offer ? remoteDesc
	                                                                : localDesc;
	auto answer = remoteDesc.type() == rtc::Description::Type::Offer
	                  ? localDesc
	                  : remoteDesc;

	for (int i = 0; i < offer.mediaCount(); i++) {
		auto offerMedia = getMedia(offer, i);
		if (!offerMedia)
			continue;
		if (offerMedia->mid() != mid)
			continue;

		for (auto offerPt : offerMedia->payloadTypes()) {

			for (int j = 0; j < answer.mediaCount(); j++) {
				auto answerMedia = getMedia(answer, j);
				if (!answerMedia)
					continue;

				if (answerMedia->mid() != mid)
					continue;

				if (answerMedia->hasPayloadType(offerPt)) {
					return *offerMedia->rtpMap(offerPt);
				}
			}
		}
	}

	throw std::runtime_error("No matching codec found for negotiation");
}