#include "gx_zephyrplatform.h"
#include "gx_coreapplication.h"
#include "gx_zephyrlcddriver.h"
#include "gx_touchevent.h"

using namespace gx;

PersimPlatform::PersimPlatform(struct device *device) : PlatformAdapter(new LCDDevice(device)) {}

PersimPlatform::~PersimPlatform() {}

void PersimPlatform::processNative(CoreApplication *app, int filter) {}
