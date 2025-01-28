/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_file.h"
#include "gx_sharedref.h"
#include "gx_string.h"
#include "platform/gx_os.h"
#include <cstddef>

namespace gx {
class Package : public SharedValue {
public:
    struct Record {
        uint32_t start, size, offset;
    };
    class Reader;
    class FileReader;
    class DirectReader;

    Package(const String &name, Reader *reader);
    Package(const String &name, const String &path);
    virtual ~Package();
    GX_NODISCARD bool isValid() const { return m_valid; }
    GX_NODISCARD bool isLoaded() const { return m_loaded; }
    GX_NODISCARD bool isOutdated() const { return m_outdated; }
    GX_NODISCARD const String &packageName() const { return m_name; }
    GX_NODISCARD const String &packagePath() const { return m_path; }
    void setOutdated();
    GX_NODISCARD bool exists(const char *path);
    GX_NODISCARD bool open(Record *rec, const char *path);
    bool close(Record *rec) { return rec; }
    int read(Record *rec, void *buffer, std::size_t length);
    os::off_t seek(Record *rec, os::off_t offset, int whence);
    GX_NODISCARD const void *rawptr(Record *rec);
    void release();
    /**
     * Get the pointer to user-defined data.
     */
    GX_NODISCARD void *opaque() const { return m_opaque; }
    /**
     * Set a user-defined data pointer whose data is maintained by the user.
     */
    void setOpaque(void *opaque) { m_opaque = opaque; }

    struct CacheHelper {
        typedef String key_type;
        typedef std::size_t size_type;
        key_type key(const Package &pkg) { return pkg.packageName(); }
        size_type size(const Package &pkg) { return pkg.isLoaded(); }
        size_type release(Package &pkg, size_type);
    };

private:
    void load();

private:
    struct Entry;
    String m_name, m_path;
    Reader *m_reader;
    void *m_opaque;
    Entry *m_entries;
    uint32_t m_entryCount;
    uint32_t m_valid : 1;
    uint32_t m_loaded : 1;
    uint32_t m_outdated : 1;
};

class Package::Reader {
public:
    Reader() GX_DEFAULT;
    virtual ~Reader() GX_DEFAULT;

    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual int read(void *buffer, std::size_t length) = 0;
    virtual os::off_t seek(os::off_t offset, int whence) = 0;
    virtual const void *rawptr() = 0;
};

class Package::FileReader : public Reader {
public:
    FileReader(const String &uri);
    virtual ~FileReader();

    virtual bool open();
    virtual bool close();
    virtual int read(void *buffer, std::size_t length);
    virtual os::off_t seek(os::off_t offset, int whence);
    virtual const void *rawptr();

private:
    File m_file;
};

class Package::DirectReader : public Reader {
public:
    explicit DirectReader(const String &path);
    virtual ~DirectReader();

    virtual bool open();
    virtual bool close();
    virtual int read(void *buffer, std::size_t length);
    virtual os::off_t seek(os::off_t offset, int whence);
    virtual const void *rawptr();

private:
    String m_path;
    os::file_t m_handle;
};

inline bool Package::exists(const char *path) {
    Record rec;
    return open(&rec, path);
}
} // namespace gx
