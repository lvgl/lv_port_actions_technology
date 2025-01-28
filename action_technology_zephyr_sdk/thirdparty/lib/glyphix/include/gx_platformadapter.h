/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
class PaintDevice;
class CoreApplication;
struct PaintDeviceMetric;

class PlatformAdapter {
public:
    explicit PlatformAdapter(PaintDevice *screen = nullptr);
    virtual ~PlatformAdapter();
    GX_NODISCARD PaintDevice *screenDevice() const;
    virtual void screenMetrics(PaintDeviceMetric *metric) const;
    virtual void processNative(CoreApplication *app, int filter);
    GX_NODISCARD virtual uint32_t currentTick();

private:
    PaintDevice *m_screenDevice;
};
} // namespace gx
