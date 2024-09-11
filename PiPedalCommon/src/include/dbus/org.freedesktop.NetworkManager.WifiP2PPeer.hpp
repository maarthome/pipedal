
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_WifiP2PPeer_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_WifiP2PPeer_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {

class WifiP2PPeer_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.WifiP2PPeer";

protected:
    WifiP2PPeer_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~WifiP2PPeer_proxy() = default;

public:
    std::string Name()
    {
        return proxy_.getProperty("Name").onInterface(INTERFACE_NAME);
    }

    uint32_t Flags()
    {
        return proxy_.getProperty("Flags").onInterface(INTERFACE_NAME);
    }

    std::string Manufacturer()
    {
        return proxy_.getProperty("Manufacturer").onInterface(INTERFACE_NAME);
    }

    std::string Model()
    {
        return proxy_.getProperty("Model").onInterface(INTERFACE_NAME);
    }

    std::string ModelNumber()
    {
        return proxy_.getProperty("ModelNumber").onInterface(INTERFACE_NAME);
    }

    std::string Serial()
    {
        return proxy_.getProperty("Serial").onInterface(INTERFACE_NAME);
    }

    std::vector<uint8_t> WfdIEs()
    {
        return proxy_.getProperty("WfdIEs").onInterface(INTERFACE_NAME);
    }

    std::string HwAddress()
    {
        return proxy_.getProperty("HwAddress").onInterface(INTERFACE_NAME);
    }

    uint8_t Strength()
    {
        return proxy_.getProperty("Strength").onInterface(INTERFACE_NAME);
    }

    int32_t LastSeen()
    {
        return proxy_.getProperty("LastSeen").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif
