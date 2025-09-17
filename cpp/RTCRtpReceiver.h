#pragma once
#include "MediaStreamTrack.h"
#include <rtc/rtc.hpp>

std::shared_ptr<rtc::Track>
addTransceiver(std::shared_ptr<rtc::PeerConnection> peerConnection, int index,
               const std::string &kind, rtc::Description::Direction direction,
               const std::string &sendms, const std::string &recvms,
               const std::vector<std::string> &msids,
               const std::optional<std::string> &trackid);