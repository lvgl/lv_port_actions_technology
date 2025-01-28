/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"
#include "platform/gx_os.h"
#include <utility>

namespace gx {
/**
 * Provides support for path and directory operations.
 */
class Dir {
public:
    //! The filtering options for traversing the directory. Use `|` to unite options.
    enum Filter {
        Normal = 0,      //!< There are no excluded items.
        NoDot = 1,       //!< The `. ` and `..` will be excluded.
        NoFile = 2,      //!< The file will be excluded.
        NoDirectory = 4, //!< The directory will be excluded.
    };
    GX_FLAGS(Filter);

    class List {
    public:
        explicit List(const Dir &dir, Filter filter = Normal);
        ~List();
        GX_NODISCARD bool hasNext() const { return m_next; }
        void next();
        GX_NODISCARD String name() const { return m_info.name; }
        GX_NODISCARD bool isFile() const { return m_info.type == os::FT_REG; }
        GX_NODISCARD bool isDirectory() const { return m_info.type == os::FT_DIR; }

    private:
        bool m_next;
        uint8_t m_filter;
        os::dirinfo_t m_info;
    };

#ifdef GX_COPMATIBLE_CXX_98
    explicit Dir(const String &path = String()) : m_path(path) {}
#else
    explicit Dir(String path = String()) : m_path(std::move(path)) {}
#endif

    GX_NODISCARD bool isAbsolute() const;
    GX_NODISCARD bool isFile() const;
    GX_NODISCARD bool isDirectory() const;
    GX_NODISCARD Dir join(const String &path) const;
    GX_NODISCARD String path() const { return m_path; }
    void setPath(const String &path) { m_path = path; }
    GX_NODISCARD String absolutePath() const;
    GX_NODISCARD String directoryName() const;
    GX_NODISCARD String fileName() const;
    GX_NODISCARD String baseName() const { return splitExtension(fileName()).first; }
    GX_NODISCARD String extName() const { return splitExtension(fileName()).second; }
    /**
     * Returns the normalize path, i.e. a path without symbolic links or redundant "." or ".."
     * elements. For example ``/a/.../b/.//c`` would be reduced to ``/a/c``, while ``a/.../.../b``
     * would be reduced to ``.../b``.
     * @see normalizePath(const String &)
     */
    GX_NODISCARD String normalizePath() const { return normalizePath(path()); }
    /**
     * Returns the path relative to the directory. For examples:
     * @code
     * Dir dir("/usr/bin");
     * dir.relativePath("local/file.txt");      // "local/file.txt"
     * dir.relativePath("/usr/local/file.txt"); // "../local/file.txt"
     * @endcode
     */
    GX_NODISCARD String relativePath(const String &path) const;

    GX_NODISCARD bool exists(const String &path = String()) const;
    bool mkdir(const String &path = String()) const;                 // NOLINT(*-use-nodiscard)
    bool mkpath(const String &path = String()) const;                // NOLINT(*-use-nodiscard)
    bool remove(const String &name = String()) const;                // NOLINT(*-use-nodiscard)
    bool rmpath(const String &name = String()) const;                // NOLINT(*-use-nodiscard)
    bool rename(const String &oldName, const String &newName) const; // NOLINT(*-use-nodiscard)
    GX_NODISCARD List listdir(Filter filter = Normal) const { return List(*this, filter); }

    GX_NODISCARD static String currentPath();
    /**
     * Returns the normalize path, i.e. a path without symbolic links or redundant `.` or `..`
     * elements. For example `/a/../b/.//c` would be reduced to `/a/c`, while `a/../../b` would
     * be reduced to `../b`.
     * @see normalizePath()
     * @param path The path string to be normalized.
     */
    GX_NODISCARD static String normalizePath(const String &path);
    GX_NODISCARD static bool isAbsolute(const String &path);
    GX_NODISCARD static std::pair<String, String> splitExtension(const String &path);

private:
    String m_path;
};
} // namespace gx
