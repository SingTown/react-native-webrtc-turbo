#pragma once
#include <optional>
#include <rtc/rtc.hpp>

std::string extractFmtpStringValue(const std::string &fmtp,
                                   const std::string &key);
int extractFmtpIntValue(const std::string &fmtp, const std::string &key,
                        int base);
int getH264PacketizationMode(const std::vector<std::string> &fmtps);
int getH264LevelAsymmetryAllowed(const std::vector<std::string> &fmtps);
int getH264ProfileId(const std::vector<std::string> &fmtps);
int getH264ConstrainedId(const std::vector<std::string> &fmtps);
int getH264LevelId(const std::vector<std::string> &fmtps);
int getH265ProfileId(const std::vector<std::string> &fmtps);
int getH265TierFlag(const std::vector<std::string> &fmtps);
int getH265LevelId(const std::vector<std::string> &fmtps);

std::optional<rtc::Description::Media>
getMediaFromIndex(const rtc::Description &description, int index);

std::optional<rtc::Description::Media>
getMediaFromMid(const rtc::Description &description, const std::string &mid);

bool isRtpMapMatchExceptPayloadType(const rtc::Description::Media::RtpMap *a,
                                    const rtc::Description::Media::RtpMap *b);
std::string getFmtpsString(const std::vector<std::string> &fmtps);
rtc::Description::Media
getSupportedMedia(const std::string &mid, rtc::Description::Direction dir,
                  const std::string &kind,
                  const std::vector<std::string> &msids,
                  const std::optional<std::string> &trackid);
std::optional<rtc::Description::Media>
negotiateAnswerMedia(const rtc::Description &offer, int index,
                     rtc::Description::Direction direction,
                     const std::string &kind,
                     const std::vector<std::string> &msids,
                     const std::optional<std::string> &trackid);

std::optional<rtc::Description::Media::RtpMap>
negotiateRtpMap(const rtc::Description &remoteDesc,
                const rtc::Description &localDesc, const std::string &mid);
