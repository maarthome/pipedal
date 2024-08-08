#pragma once

#include "DBusEvent.hpp"
#include "DBusLog.hpp"
#include "DBusDispatcher.hpp"

#include "dbus/hpp/org.freedesktop.NetworkManager.Device.WifiP2P.hpp"
#include "dbus/hpp/org.freedesktop.NetworkManager.Device.hpp"
#include "dbus/hpp/org.freedesktop.NetworkManager.WifiP2PPeer.hpp"
#include "dbus/hpp/org.freedesktop.NetworkManager.hpp"

namespace impl {
    extern std::string NetworkManagerStateToString(uint32_t state);
    std::string NetworkManagerDeviceStateToString(uint32_t state);
};

class NetworkManager : public sdbus::ProxyInterfaces<org::freedesktop::NetworkManager_proxy>
{
public:
    using ptr = std::unique_ptr<NetworkManager>;
    NetworkManager(DBusDispatcher &dispatcher)
        : sdbus::ProxyInterfaces<org::freedesktop::NetworkManager_proxy>(
              dispatcher.Connection(),
              INTERFACE_NAME,
              "/org/freedesktop/NetworkManager")
    {
        registerProxy();
    }
    virtual ~NetworkManager()
    {
        unregisterProxy();
    }

    static ptr Create(DBusDispatcher &dispatcher) { return std::make_unique<NetworkManager>(dispatcher); }
    DBusEvent<> OnCheckPermissions;
    DBusEvent<uint32_t> OnStateChanged;
    DBusEvent<const sdbus::ObjectPath &> OnDeviceAdded;
    DBusEvent<const sdbus::ObjectPath &> OnDeviceRemoved;

private:
    void EventTrace(const char*method, const std::string &message) {
        LogTrace(getObjectPath(),method,message);
    }
    virtual void onStateChanged(const uint32_t &state) { 
        EventTrace("onStateChanged",impl::NetworkManagerStateToString(state));
        OnStateChanged.fire(state); 
    }

    virtual void onCheckPermissions() { OnCheckPermissions.fire(); }

    virtual void onDeviceAdded(const sdbus::ObjectPath &device_path) { 
        EventTrace("onDeviceAdded",device_path.c_str());
        OnDeviceAdded.fire(device_path); 
    }
    virtual void onDeviceRemoved(const sdbus::ObjectPath &device_path) { 
        EventTrace("onDeviceRemoved",device_path.c_str());

        OnDeviceRemoved.fire(device_path); 
    }
};

class Device : public sdbus::ProxyInterfaces<org::freedesktop::NetworkManager::Device_proxy>
{
public:
    using ptr = std::unique_ptr<Device>;
    Device(DBusDispatcher &dispatcher, const sdbus::ObjectPath &path)
        : sdbus::ProxyInterfaces<org::freedesktop::NetworkManager::Device_proxy>(
              dispatcher.Connection(),
              "org.freedesktop.NetworkManager",
              path)
    {
        registerProxy();
    }
    virtual ~Device()
    {
        unregisterProxy();
    }

    static ptr Create(DBusDispatcher &dispatcher, const sdbus::ObjectPath &path)
    {
        return std::make_unique<Device>(dispatcher, path);
    }
    DBusEvent<uint32_t, uint32_t, uint32_t> OnStateChanged;

private:
    const uint32_t NM_DEVICE_TYPE_WIFI_P2P = 30; // from lbnm-dev package.
public:
    bool IsP2pDevice()
    {
        return this->DeviceType() == NM_DEVICE_TYPE_WIFI_P2P;
    }

private:
    void EventTrace(const char*method, const std::string &message) {
        LogTrace(getObjectPath(),method,message);
    }

    virtual void onStateChanged(const uint32_t &new_state, const uint32_t &old_state, const uint32_t &reason)
    {
        EventTrace("onStateChanged",impl::NetworkManagerDeviceStateToString(new_state));

        OnStateChanged.fire(new_state, old_state, reason);
    }
};
class WifiP2P : public sdbus::ProxyInterfaces<
    org::freedesktop::NetworkManager::Device::WifiP2P_proxy
     >
{
public:
    using ptr = std::unique_ptr<WifiP2P>;
    using base = sdbus::ProxyInterfaces<
    org::freedesktop::NetworkManager::Device::WifiP2P_proxy
     >; 

    WifiP2P(DBusDispatcher &dispatcher, const std::string &objectPath)
        : base(dispatcher.Connection(), "org.freedesktop.NetworkManager", objectPath)
    {
        registerProxy();
    }
    virtual ~WifiP2P()
    {
        unregisterProxy();
    }
    static ptr Create(DBusDispatcher &dispatcher, const std::string &objectPath)
    {
        return std::make_unique<WifiP2P>(dispatcher, objectPath);
    }
    DBusEvent<const sdbus::ObjectPath &> OnPeerAdded;
    DBusEvent<const sdbus::ObjectPath &> OnPeerRemoved;
    DBusEvent<uint32_t> OnStateChanged;

private:
    void EventTrace(const char*method, const std::string &message) {
        LogTrace(getObjectPath(),method,message);
    }
    virtual void onStateChanged(const uint32_t &new_state, const uint32_t &old_state, const uint32_t &reason)
    {
        EventTrace("onStateChanged",impl::NetworkManagerStateToString(new_state));

        OnStateChanged.fire(new_state);
    }

    virtual void onPeerAdded(const sdbus::ObjectPath &peer)
    {
        EventTrace("onPeerAdded",peer);
        OnPeerAdded.fire(peer);
    }
    virtual void onPeerRemoved(const sdbus::ObjectPath &peer)
    {
        EventTrace("onPeerRemoved",peer);
        OnPeerRemoved.fire(peer);
    }
};

class WifiP2PPeer : public sdbus::ProxyInterfaces<org::freedesktop::NetworkManager::WifiP2PPeer_proxy>
{
public:
    using self=WifiP2PPeer;
    using ptr = std::unique_ptr<self>;

    WifiP2PPeer(DBusDispatcher &dispatcher, const sdbus::ObjectPath &objectPath)
        : sdbus::ProxyInterfaces<org::freedesktop::NetworkManager::WifiP2PPeer_proxy>(
              dispatcher.Connection(), "org.freedesktop.NetworkManager", objectPath)
    {
        registerProxy();
    }
    virtual ~WifiP2PPeer()
    {
        unregisterProxy();
    }
    static ptr Create(DBusDispatcher &dispatcher, const sdbus::ObjectPath &objectPath)
    {
        return std::make_unique<self>(dispatcher, objectPath);
    }
    DBusEvent<const sdbus::ObjectPath &> OnPeerAdded;
    DBusEvent<const sdbus::ObjectPath &> OnPeerRemoved;

private:
    virtual void onPeerAdded(const sdbus::ObjectPath &peer)
    {
        OnPeerAdded.fire(peer);
    }
    virtual void onPeerRemoved(const sdbus::ObjectPath &peer)
    {
        OnPeerRemoved.fire(peer);
    }
};

