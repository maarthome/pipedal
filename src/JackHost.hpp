#pragma once

#include "JackConfiguration.hpp"

#include "Lv2PedalBoard.hpp"
#include "VuUpdate.hpp"
#include "RingBuffer.hpp"
#include "json.hpp"
#include "JackServerSettings.hpp"
#include <functional>

namespace pipedal {



using PortMonitorCallback = std::function<void (int64_t handle, float value)>;


class MonitorPortUpdate
{
public:
    PortMonitorCallback *callbackPtr; // pointer because function<>'s probably aren't POD.
    int64_t subscriptionHandle;
    float value;
};
class RealtimeParameterRequest {
public:
    int64_t clientId;
    int64_t instanceId;
    LV2_URID uridUri;

    std::function<void (RealtimeParameterRequest*)> onJackRequestComplete;
    std::function<void(const std::string &jsonResjult)> onSuccess;
    std::function<void(const std::string &error)> onError;

    const char*errorMessage = nullptr;
    int responseLength = 0;
    uint8_t response[2048];
    std::string jsonResponse;

    RealtimeParameterRequest *pNext = nullptr;

public:
    RealtimeParameterRequest(
        std::function<void (RealtimeParameterRequest*)> onJackRequestComplete_,
        int64_t clientId_,
        int64_t instanceId_,
        LV2_URID uridUri_,
        std::function<void(const std::string &jsonResjult)> onSuccess_,
        std::function<void(const std::string &error)> onError_)
        :   onJackRequestComplete(onJackRequestComplete_),
             clientId(clientId_),
            instanceId(instanceId_),
            uridUri(uridUri_),
            onSuccess(onSuccess_),
            onError(onError_)
        {
            
        }

};


class MonitorPortSubscription {
public:
    int64_t subscriptionHandle;
    int64_t instanceid;
    std::string key;
    float updateInterval;
    PortMonitorCallback onUpdate;

};

class IJackHostCallbacks {
public:
    virtual void OnNotifyVusSubscription(const std::vector<VuUpdate> & updates) = 0;
    virtual void OnNotifyMonitorPort(const MonitorPortUpdate &update) = 0;
    virtual void OnNotifyMidiValueChanged(int64_t instanceId, int portIndex, float value) = 0;
    virtual void OnNotifyMidiListen(bool isNote, uint8_t noteOrControl) = 0;


};


class JackHostStatus {
public:
    bool active_;
    bool restarting_;
    uint64_t underruns_;
    float cpuUsage_ = 0;
    uint64_t msSinceLastUnderrun_ = 0;
    int32_t temperaturemC_ = -100000;

    DECLARE_JSON_MAP(JackHostStatus);


};

class IHost;

class JackHost {

protected: 
    JackHost() { }
public: 
    static JackHost*CreateInstance(IHost *pHost);
    virtual ~JackHost() { };

    virtual void UpdateServerConfiguration(const JackServerSettings & jackServerSettings,
        std::function<void(bool success, const std::string&errorMessage)> onComplete) = 0;

    virtual void SetNotificationCallbacks(IJackHostCallbacks *pNotifyCallbacks) = 0;

    virtual void SetListenForMidiEvent(bool listen) = 0;


    virtual void Open(const JackChannelSelection & channelSelection) = 0;
    virtual void Close() = 0;

    virtual uint32_t GetSampleRate() = 0;

    virtual JackConfiguration GetServerConfiguration() = 0;

    virtual void SetPedalBoard(const std::shared_ptr<Lv2PedalBoard> &pedalBoard) = 0;

    virtual void SetControlValue(long instanceId,const std::string&symbol, float value) = 0;
    virtual void SetPluginPreset(long isntanceId, const std::vector<ControlValue> & values) = 0;
    virtual void SetBypass(long instanceId, bool enabled) = 0;

    virtual bool IsOpen() const = 0;

    virtual void SetVuSubscriptions(const std::vector<int64_t> &instanceIds) = 0;
    virtual void SetMonitorPortSubscriptions(const std::vector<MonitorPortSubscription> &subscriptions) = 0;

    virtual void getRealtimeParameter(RealtimeParameterRequest*pParameterRequest) = 0;

    virtual JackHostStatus getJackStatus() = 0;


};




} //namespace pipedal.