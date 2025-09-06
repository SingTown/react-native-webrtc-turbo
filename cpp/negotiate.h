#pragma once
#include <optional>
#include <rtc/rtc.hpp>

std::string extractFmtpStringValue(const std::string &fmtp,
                                   const std::string &key);
int extractFmtpIntValue(const std::string &fmtp, const std::string &key,
                        int base);
int getFmtpsProfileLevelId(const std::vector<std::string> &fmtps);
int getFmtpsPacketizationMode(const std::vector<std::string> &fmtps);
int getFmtpsLevelAsymmetryAllowed(const std::vector<std::string> &fmtps);
int getProfileId(int profileLevelId);
int getConstrainedId(int profileLevelId);
int getLevelId(int profileLevelId);
bool isRtpMapMatchExceptPayloadType(const rtc::Description::Media::RtpMap *a,
                                    const rtc::Description::Media::RtpMap *b);
std::string getFmtpsString(const std::vector<std::string> &fmtps);
rtc::Description::Media getSupportedMedia(const std::string &mid,
                                          rtc::Description::Direction dir,
                                          const std::string &kind);
std::optional<rtc::Description::Media>
negotiateAnswerMedia(const rtc::Description &remoteDesc, int index,
                     rtc::Description::Direction direction,
                     const std::string &kind);

rtc::Description::Media::RtpMap
negotiateRtpMap(const rtc::Description &remoteDesc,
                const rtc::Description::Media &trackMedia);
