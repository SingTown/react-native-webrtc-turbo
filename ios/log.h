#pragma once
#include <cstdio>
#define LOG_TAG "Webrtc"
#define LOGI(...) fprintf(stdout, __VA_ARGS__)
#define LOGD(...) fprintf(stdout, __VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)