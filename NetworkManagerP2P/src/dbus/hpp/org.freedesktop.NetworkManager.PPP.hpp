
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_PPP_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_PPP_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {

class PPP_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.PPP";

protected:
    PPP_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~PPP_proxy() = default;

public:
    std::tuple<std::string, std::string> NeedSecrets()
    {
        std::tuple<std::string, std::string> result;
        proxy_.callMethod("NeedSecrets").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    void SetIp4Config(const std::map<std::string, sdbus::Variant>& config)
    {
        proxy_.callMethod("SetIp4Config").onInterface(INTERFACE_NAME).withArguments(config);
    }

    void SetIp6Config(const std::map<std::string, sdbus::Variant>& config)
    {
        proxy_.callMethod("SetIp6Config").onInterface(INTERFACE_NAME).withArguments(config);
    }

    void SetState(const uint32_t& state)
    {
        proxy_.callMethod("SetState").onInterface(INTERFACE_NAME).withArguments(state);
    }

    void SetIfindex(const int32_t& ifindex)
    {
        proxy_.callMethod("SetIfindex").onInterface(INTERFACE_NAME).withArguments(ifindex);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif