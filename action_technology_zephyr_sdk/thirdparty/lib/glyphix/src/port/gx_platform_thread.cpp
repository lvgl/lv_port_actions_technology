/*
 * This file is part of PersimUI
 * Copyright (c) 2006-2023, RT-Thread Development Team
 */
#include "platform/gx_thread.h"
#include <posix/pthread.h>

namespace gx {
namespace os {
void *this_thread() { 
    return pthread_self(); 
}
} // namespace os
} // namespace gx
