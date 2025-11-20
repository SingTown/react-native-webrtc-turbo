#pragma once
#include "ffmpeg.h"
#include <functional>

using FrameCallback = std::function<void(std::shared_ptr<AVFrame> frame)>;
using CleanupCallback = std::function<void()>;

int subscribe(const std::string &pipeId, FrameCallback onFrame,
              CleanupCallback onCleanup = {});
void unsubscribe(int subscriptionId);
void publish(const std::string &pipeId, std::shared_ptr<AVFrame> frame);
