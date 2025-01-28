/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "platform/gx_os.h"
#include "gx_string.h"

namespace gx {
class AbstractStream {
public:
    virtual ~AbstractStream() GX_DEFAULT;
    virtual int open(const String &uri) = 0;
    virtual os::off_t size() = 0;
    virtual os::off_t tell() = 0;
    virtual os::off_t seek(os::off_t pos) = 0;
    virtual os::off_t seekByWhence(os::off_t pos, int whence) = 0;
    virtual size_t read(void *data, size_t count) = 0;
};
} // namespace gx
