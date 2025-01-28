/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstract_httpclient.h"
#include "gx_async.h"
#include "gx_hashmap.h"
#include "gx_mutex.h"
#include "gx_string.h"

#include <gx_vector.h>
#define GX_FETCH_DEFAULT_TIMEOUT (6 * 1000)

namespace gx {
class AbstractStream;

enum NetworkType {
    NetworkTypeNone = 0,
    NetworkTypeBluetooth,
    NetworkTypeMobile,
    NetworkTypeWifi,
    NetworkTypeEthernet,
    NetworkTypeOthers = 100,
};

enum NetworkEvent { NetworkLinkUp, NetworkLinkDown };

class NetworkDevice {
public:
    virtual ~NetworkDevice() GX_DEFAULT;

    const String &name() const { return m_name; }
    NetworkType type() const { return NetworkType(m_type); }
    virtual bool isUp() const { return true; }
    virtual bool isLinkUp() const { return true; }
    virtual bool isOnline() const { return true; };
    virtual AbstractHttpClient *createHttpClient(size_t headerSize) = 0;

protected:
    NetworkDevice(const String &name, int type) : m_name(name), m_type(type) {}

private:
    String m_name;
    uint8_t m_type;
};

struct NetworkStatus {
    NetworkDevice *device;
    NetworkEvent event;
    NetworkStatus(NetworkDevice *device, NetworkEvent event) : device(device), event(event) {}
    operator JsValue() const;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    NetworkDevice *selectedDevice();

    bool install(NetworkDevice *device);
    bool uninstall(const String &name);
    NetworkDevice *deivce() const;
    const HashMap<String, NetworkDevice *> &devices() const { return m_devices; }
    bool selectDevice(String name);
    void postNetworkEvent(NetworkDevice *device, NetworkEvent event);

    async::Signal<NetworkStatus> networkEvent;

    static NetworkManager *instance() { return m_instance; }

private:
    void deviceCheck();

    static NetworkManager *m_instance;
    NetworkDevice *m_selectedDevice;
    HashMap<String, NetworkDevice *> m_devices;
    Mutex m_mutex;
};

const Logger &operator<<(const Logger &, const NetworkDevice *);
} // namespace gx
