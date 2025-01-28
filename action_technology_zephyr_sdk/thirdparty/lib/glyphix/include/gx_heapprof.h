/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_filelogger.h"
#include "gx_string.h"

namespace gx {
class FileLogger;

class HeapProfiler {
public:
    HeapProfiler();
    void setLogFile(const String &path);
    void trackWidgetTree();

private:
    FileLoggerStream m_stream;
};
} // namespace gx
