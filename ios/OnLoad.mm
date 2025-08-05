#import <Foundation/Foundation.h>
#import "DatachannelImpl.h"
#import <ReactCommon/CxxTurboModuleUtils.h>


@interface OnLoad : NSObject
@end

@implementation OnLoad

using namespace facebook::react;

+ (void) load {
  registerCxxModuleToGlobalModuleMap(
    std::string(DatachannelImpl::kModuleName),
    [](std::shared_ptr<CallInvoker> jsInvoker) {
      return std::make_shared<DatachannelImpl>(jsInvoker);
    }
  );
}

@end