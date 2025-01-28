/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once
#include "gx_async.h"

namespace gx {
struct Location {
    int code;
    String msg;
    double longitude; // 经度
    double latitude;  // 纬度
    String coordType; // 坐标系类型
    Location(int code = 0) : code(code){};
    Location(int code, String msg, double lon, double lat, String type)
        : code(code), msg(msg), longitude(lon), latitude(lat), coordType(type) {}
    operator JsValue() const;
};

class GeoLocation {
public:
    explicit GeoLocation();
    ~GeoLocation() { m_instance = nullptr; }
    async::Signal<Location> locationEvent;

    void postLocationEvent(int code, String msg, double longitude, double latitude, String type);

    Location getLocation(String params);

    static GeoLocation *instance() { return m_instance; }

private:
    static GeoLocation *m_instance;
    Mutex m_mutex;
};
} // namespace gx
