#pragma once

#include <RNDatachannelTurboModuleSpecJSI.h>

#include <memory>
#include <string>
#include <vector>

namespace facebook::react
{
    using LocalDescription = NativeDatachannelTurboModuleLocalDescription<int, std::string, std::string>;
    using LocalCandidate = NativeDatachannelTurboModuleLocalCandidate<int, std::string, std::string>;
    template <>
    struct Bridging<LocalDescription>
        : NativeDatachannelTurboModuleLocalDescriptionBridging<LocalDescription>
    {
    };
    template <>
    struct Bridging<LocalCandidate>
        : NativeDatachannelTurboModuleLocalCandidateBridging<LocalCandidate>
    {
    };

    class DatachannelImpl : public NativeDatachannelTurboModuleCxxSpec<DatachannelImpl>
    {
    public:

        DatachannelImpl(std::shared_ptr<CallInvoker> jsInvoker);

        int createPeerConnection(jsi::Runtime &rt, std::vector<std::string> servers);
        void closePeerConnection(jsi::Runtime &rt, int pc);
        void deletePeerConnection(jsi::Runtime &rt, int pc);

        void setLocalDescription(jsi::Runtime &rt, int pc, std::string type);
        std::string getLocalDescription(jsi::Runtime &rt, int pc);

        void setRemoteDescription(jsi::Runtime &rt, int pc, std::string sdp, std::string type);
        std::string getRemoteDescription(jsi::Runtime &rt, int pc);

        int addTrack(jsi::Runtime &rt, int pc, std::string sdp);
        void deleteTrack(jsi::Runtime &rt, int tr);

        void addRemoteCandidate(jsi::Runtime &rt, int pc, std::string candidate, std::string mid);

        void onLocalDescription(int pc, std::string sdp, std::string type);
        void onLocalCandidate(int pc, std::string candidate, std::string mid);
    };

}
