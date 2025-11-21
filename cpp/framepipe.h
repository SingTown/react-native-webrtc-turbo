#pragma once
#include "ffmpeg.h"
#include <functional>

using FrameCallback = std::function<void(std::string pipeId, int subscriptionId,
                                         std::shared_ptr<AVFrame> frame)>;
using CleanupCallback = std::function<void(int subscriptionId)>;

int subscribe(const std::vector<std::string> &pipeIds, FrameCallback onFrame,
              CleanupCallback onCleanup = {});
void unsubscribe(int subscriptionId);
void publish(const std::string &pipeId, std::shared_ptr<AVFrame> frame);
