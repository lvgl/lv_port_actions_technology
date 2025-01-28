/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_notification.h"

#define GX_LOG_TAG "notification"
#define GX_LOG_LEVEL GX_LOG_INFO
#include "gx_logger.h"

namespace gx {
void notificationPublish(const JsValue &notification) {
    LogInfo() << notification;
}
} // namespace gx
