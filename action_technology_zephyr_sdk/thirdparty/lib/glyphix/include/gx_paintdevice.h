/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_geometry.h"

namespace gx {
class PaintEngine;

struct PaintDeviceMetric {
    uint16_t width;
    uint16_t height;
    uint16_t dpi;
    uint8_t pixelFormat;
    uint8_t shape;

    int top() const { return 0; }  // NOLINT(*-convert-member-functions-to-static)
    int left() const { return 0; } // NOLINT(*-convert-member-functions-to-static)
    int bottom() const { return int(height) - 1; }
    int right() const { return int(width) - 1; }
    Size size() const { return Size(width, height); }
    Rect rect() const { return Rect(Point(), size()); }
    Point center() const { return Point(width / 2, height / 2); }
};

class PaintDevice {
public:
    //! The shape of the screen or paint device.
    enum Shape {
        Rectangle, //! Rectangular screen or paint device.
        Round      //! Round screen or paint device.
    };
    typedef PaintDeviceMetric Metric;
    virtual ~PaintDevice();

    GX_NODISCARD Size size() const;
    GX_NODISCARD Rect rect() const { return Rect(Point(), size()); }
    GX_NODISCARD virtual PaintEngine *paintEngine() = 0;
    virtual void metrics(Metric *metric) const = 0;
    virtual void start();
    virtual void flush(const Rect &rect);

protected:
    PaintDevice();
};

inline Size PaintDevice::size() const {
    Metric m;
    metrics(&m);
    return Size(m.width, m.height);
}
} // namespace gx
