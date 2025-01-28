#include "gx_zephyrlcddriver.h"
#include "board_cfg.h"
#include "device.h"
#include "kernel.h"
#include <device.h>
#include "gx_logger.h"
#include "logging/log.h"
using namespace gx;

#define X_RES       (CONFIG_PANEL_HOR_RES)
#define Y_RES       (CONFIG_PANEL_VER_RES)
#define VDB_SIZE    (X_RES * Y_RES * 2)
#define NUM_LAYERS  (1)

extern __attribute__((aligned(64))) uint8_t bg_vdb[VDB_SIZE];

static int get_pixel_format(int fmt) {
    switch (fmt) {
    case 1: return PixelFormat::RGB888;
    case 2: return PixelFormat::ARGB8888;
    case 3: return PixelFormat::RGB565;
    }
    return -1;
}

LCDDevice::LCDDevice(const struct device *device) {;
    m_lcd_panel = device ? device : device_get_binding("xxx");
    /* get device info */
    data.width = X_RES;
    data.height = Y_RES;
    data.pixels = bg_vdb;
    data.pixfmt = get_pixel_format(3);
    data.pitch = X_RES * 2;
}

LCDDevice::~LCDDevice() {

}

extern "C" void lcd_fb_flush();

/* event loop call */
void LCDDevice::flush(const Rect &rect) {
    lcd_fb_flush();
}

void LCDDevice::metrics(Metric *metric) const {
    metric->width = width();
    metric->height = height();
    metric->dpi = 200;
    metric->pixelFormat = pixelFormat();
    metric->shape = PaintDevice::Round;
}
