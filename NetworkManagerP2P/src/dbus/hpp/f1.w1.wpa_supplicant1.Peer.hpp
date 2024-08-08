
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_f1_w1_wpa_supplicant1_Peer_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_f1_w1_wpa_supplicant1_Peer_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace fi {
namespace w1 {
namespace wpa_supplicant1 {

class Peer_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "fi.w1.wpa_supplicant1.Peer";

protected:
    Peer_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
        proxy_.uponSignal("PropertiesChanged").onInterface(INTERFACE_NAME).call([this](const std::map<std::string, sdbus::Variant>& properties){ this->onPropertiesChanged(properties); });
    }

    ~Peer_proxy() = default;

    virtual void onPropertiesChanged(const std::map<std::string, sdbus::Variant>& properties) = 0;

public:
    std::string DeviceName()
    {
        return proxy_.getProperty("DeviceName").onInterface(INTERFACE_NAME);
    }

    std::string Manufacturer()
    {
        return proxy_.getProperty("Manufacturer").onInterface(INTERFACE_NAME);
    }

    std::string ModelName()
    {
        return proxy_.getProperty("ModelName").onInterface(INTERFACE_NAME);
    }

    std::string ModelNumber()
    {
        return proxy_.getProperty("ModelNumber").onInterface(INTERFACE_NAME);
    }

    std::string SerialNumber()
    {
        return proxy_.getProperty("SerialNumber").onInterface(INTERFACE_NAME);
    }

    std::vector<uint8_t> PrimaryDeviceType()
    {
        return proxy_.getProperty("PrimaryDeviceType").onInterface(INTERFACE_NAME);
    }

    uint16_t config_method()
    {
        return proxy_.getProperty("config_method").onInterface(INTERFACE_NAME);
    }

    int32_t level()
    {
        return proxy_.getProperty("level").onInterface(INTERFACE_NAME);
    }

    uint8_t devicecapability()
    {
        return proxy_.getProperty("devicecapability").onInterface(INTERFACE_NAME);
    }

    uint8_t groupcapability()
    {
        return proxy_.getProperty("groupcapability").onInterface(INTERFACE_NAME);
    }

    std::vector<std::vector<uint8_t>> SecondaryDeviceTypes()
    {
        return proxy_.getProperty("SecondaryDeviceTypes").onInterface(INTERFACE_NAME);
    }

    std::vector<std::vector<uint8_t>> VendorExtension()
    {
        return proxy_.getProperty("VendorExtension").onInterface(INTERFACE_NAME);
    }

    std::vector<uint8_t> IEs()
    {
        return proxy_.getProperty("IEs").onInterface(INTERFACE_NAME);
    }

    std::vector<uint8_t> DeviceAddress()
    {
        return proxy_.getProperty("DeviceAddress").onInterface(INTERFACE_NAME);
    }

    std::vector<sdbus::ObjectPath> Groups()
    {
        return proxy_.getProperty("Groups").onInterface(INTERFACE_NAME);
    }

    std::vector<uint8_t> VSIE()
    {
        return proxy_.getProperty("VSIE").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif