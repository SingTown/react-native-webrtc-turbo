#pragma once

#include <WebrtcSpecJSI.h>

#include <memory>
#include <string>

namespace facebook::react {

class NativeDatachannelModule
    : public NativeDatachannelModuleCxxSpec<NativeDatachannelModule> {
  public:
	NativeDatachannelModule(std::shared_ptr<CallInvoker> jsInvoker);

	std::string reverseString(jsi::Runtime &rt, std::string input);
};

} // namespace facebook::react
