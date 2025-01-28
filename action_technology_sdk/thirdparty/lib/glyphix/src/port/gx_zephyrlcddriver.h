#pragma once

#include "gx_pixmappaintdevice.h"

struct device;

class LCDDevice : public gx::PixmapPaintDevice
{
public:
    explicit LCDDevice(const struct device* device = nullptr);
    virtual ~LCDDevice();
    virtual void flush(const gx::Rect& rect);
    virtual void metrics(Metric* metric) const;
    void setFramebuffer(uint8_t* buffer) { data.pixels = buffer; };

private:
    const struct device* m_lcd_panel;
};
