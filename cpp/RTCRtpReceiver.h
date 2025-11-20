#pragma once
#include <rtc/rtc.hpp>

std::shared_ptr<rtc::Track>
addTransceiver(std::shared_ptr<rtc::PeerConnection> peerConnection, int index,
               const std::string &kind, rtc::Description::Direction direction,
               const std::string &sendPipeId, const std::string &recvPipeId,
               const std::vector<std::string> &msids,
               const std::optional<std::string> &trackid);