#include "NativeDatachannelModule.h"
#include <libavutil/avutil.h>
#include <rtc/rtc.h>

namespace facebook::react {

NativeDatachannelModule::NativeDatachannelModule(
    std::shared_ptr<CallInvoker> jsInvoker)
    : NativeDatachannelModuleCxxSpec(std::move(jsInvoker)) {}

std::string NativeDatachannelModule::reverseString(jsi::Runtime &rt,
                                                   std::string input) {
	return std::string(input.rbegin(), input.rend());
}

} // namespace facebook::react
