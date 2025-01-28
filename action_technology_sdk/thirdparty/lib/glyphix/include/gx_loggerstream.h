/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

#define GX_LOG_DEBUG 0
#define GX_LOG_INFO  1
#define GX_LOG_WARN  2
#define GX_LOG_ERROR 3
#define GX_LOG_FATAL 4

namespace gx {
class LoggerStream {
public:
    enum Features { Timestamp = 0x0001, WriteFile = 0x0002 };
    GX_FLAGS(Features);
    struct State {};
    LoggerStream() GX_DEFAULT;
    virtual ~LoggerStream() GX_DEFAULT;
    virtual void setFeature(Features features, bool enabled);
    virtual State *start(int level, const char *tag) = 0;
    virtual void puts(State *state, const char *span) = 0;
    virtual State *flush(State *state, bool end) = 0;

    static LoggerStream *stream;
};
} // namespace gx
