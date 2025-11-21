
#import "NativeDatachannelModuleProvider.h"
#import "NativeDatachannel.h"
#import <ReactCommon/CallInvoker.h>
#import <ReactCommon/TurboModule.h>

@implementation NativeDatachannelModuleProvider

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
	return std::make_shared<facebook::react::NativeDatachannel>(
	    params.jsInvoker);
}

@end
