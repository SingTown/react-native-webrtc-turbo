#pragma once
#include "MediaStreamTrack.h"
#include <rtc/rtc.hpp>

void SenderOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                  std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                  std::shared_ptr<rtc::Track> track);
void ReceiverOnOpen(std::shared_ptr<rtc::PeerConnection> peerConnection,
                    std::shared_ptr<MediaStreamTrack> mediaStreamTrack,
                    std::shared_ptr<rtc::Track> track);
std::shared_ptr<rtc::Track>
addTransceiver(std::shared_ptr<rtc::PeerConnection> peerConnection, int index,
               const std::string &kind, rtc::Description::Direction direction,
               const std::string &sendms, const std::string &recvms);