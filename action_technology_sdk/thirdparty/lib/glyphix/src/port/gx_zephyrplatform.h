#pragma once

#include "gx_geometry.h"
#include "gx_platformadapter.h"

class PersimPlatform : public gx::PlatformAdapter {
public:
    explicit PersimPlatform(struct device *device = nullptr);
    virtual ~PersimPlatform();
    virtual void processNative(gx::CoreApplication *app, int filter);
};
