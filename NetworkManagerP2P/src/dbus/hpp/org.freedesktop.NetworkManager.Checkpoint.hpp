
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_Checkpoint_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_Checkpoint_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {

class Checkpoint_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.Checkpoint";

protected:
    Checkpoint_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~Checkpoint_proxy() = default;

public:
    std::vector<sdbus::ObjectPath> Devices()
    {
        return proxy_.getProperty("Devices").onInterface(INTERFACE_NAME);
    }

    int64_t Created()
    {
        return proxy_.getProperty("Created").onInterface(INTERFACE_NAME);
    }

    uint32_t RollbackTimeout()
    {
        return proxy_.getProperty("RollbackTimeout").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif