#include "NativeDatachannelModule.h"
#include <libavutil/avutil.h>
#include <random>
#include <rtc/rtc.h>

namespace facebook::react {

NativeDatachannelModule::NativeDatachannelModule(
    std::shared_ptr<CallInvoker> jsInvoker)
    : NativeDatachannelModuleCxxSpec(std::move(jsInvoker)) {}

std::string NativeDatachannelModule::reverseString(jsi::Runtime &rt,
                                                   std::string input) {
	return std::string(input.rbegin(), input.rend());
}

std::vector<uint8_t> buffer(720 * 480 * 3);
std::vector<uint8_t> &getTrackBuffer(int tr) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint8_t> dis(0, 255);

	for (auto &byte : buffer) {
		byte = dis(gen);
	}

	return buffer;
}

} // namespace facebook::react
