/*
 * This file is part of Glyphix
 * Copyright (c) 2023-2024, Glyphix Development Team
 */

#include "gx_abstractstream.h"

namespace gx {
class MediaStream {
public:
    MediaStream();
    ~MediaStream();
    int open(const String &uri);
    os::off_t size();
    os::off_t tell();
    os::off_t seekByWhence(os::off_t pos, int whence);
    os::off_t seek(os::off_t pos);
    size_t read(void *data, size_t count);

private:
    AbstractStream *m_mediaStream = nullptr;
};
} // namespace gx
