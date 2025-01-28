/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_file.h"
#include "gx_object.h"

namespace gx {
class Uri;

class ResourceFile {
public:
    ResourceFile() GX_DEFAULT;
    virtual ~ResourceFile() GX_DEFAULT;

    virtual int read(void *buffer, std::size_t length) = 0;
    virtual int write(const void *buffer, std::size_t length) = 0;
    virtual os::off_t seek(os::off_t offset, int whence) = 0;
    virtual const void *rawptr() = 0;
    virtual void sync();
};

class ResourceInterface : public PrimitiveObject {
    GX_OBJECT

public:
    enum Status { Invalid = 0, Valid = 1, Cached = 2 };
    GX_FLAGS(Status)
    struct Location {
        String path;
        Status mode;
        Location() : mode(Invalid) {}
        Location(const String &path, Status mode) : path(path), mode(mode) {}
    };
    ResourceInterface() GX_DEFAULT;
    virtual ~ResourceInterface() GX_DEFAULT;

    virtual Location locate(const String &domain) = 0;
    virtual Status resolve(const String &package, Uri *uri) = 0;
    virtual ResourceFile *open(const Uri &uri, int flags) = 0;
    /**
     * Refresh the specified resource package. The refresh policy is implemented by a specific
     * resource interface protocol. This function is needed to clean up data that has been cached
     * and to process files that have not been closed. This function is called in
     * ResourceManager::refresh().
     * @param package Name of the resource package to be refreshed.
     * @note You don't need to deal with external insides such as font caching, they are refreshed
     * by the resource framework.
     */
    virtual void refresh(const String &package) = 0;
};
} // namespace gx
