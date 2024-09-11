
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_DHCP6Config_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_DHCP6Config_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {

class DHCP6Config_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.DHCP6Config";

protected:
    DHCP6Config_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~DHCP6Config_proxy() = default;

public:
    std::map<std::string, sdbus::Variant> Options()
    {
        return proxy_.getProperty("Options").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif
