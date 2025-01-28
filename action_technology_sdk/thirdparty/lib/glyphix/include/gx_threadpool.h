/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __PTHREAD_POOL_H__
#define __PTHREAD_POOL_H__

#include "gx_event.h"

namespace gx {
class ThreadPool {
public:
    virtual ~ThreadPool() GX_DEFAULT;
    virtual bool enqueue(TaskEvent *task) = 0;
};
} // namespace gx
#endif //__PTHREAD_POOL_H__
