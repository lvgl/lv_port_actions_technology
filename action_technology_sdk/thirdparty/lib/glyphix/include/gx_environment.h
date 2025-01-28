/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"
#include "gx_vector.h"

namespace gx {
class EnvPath {
public:
    /**
     * Resource file access permission masks. Permissions are divided into system-level and
     * application-level, and system-level permissions are higher than user-level permissions.
     * Resource permissions are usually specific to JavaScript application code, and native
     * code has full access.
     */
    enum Permission {
        //! The resource is denied access.
        AccessDenied = 0,
        //! Applications with system permissions are allowed to read.
        SystemRead = 1 << 0,
        //! Applications with system permissions are allowed to write.
        SystemWrite = 1 << 1,
        //! Applications with system permissions are allowed to read and write.
        SystemReadWrite = SystemRead | SystemWrite,
        //! Applications with user permissions are allowed to read.
        UserRead = SystemRead | 1 << 2,
        //! Applications with user permissions are allowed to write.
        UserWrite = SystemWrite | 1 << 3,
        //! Applications with user permissions are allowed to read and write.
        UserReadWrite = UserRead | UserWrite
    };

    /**
     * This enumeration type contains various types of environment paths.
     * @see entry()
     * @see setEntry()
     * @see reset()
     */
    enum Role {
        /**
         * The file path to the global resource package. This resource package resolves to the
         * global package name of the `pkg` resource protocol. Assuming the path to the global
         * package is `/global.pkg`, then `pkg:///font.ttf` will locate the `/font.ttf` within
         * that package.
         */
        GlobalPackage,
        //! The path to the cache file of the applet.
        AppletCache,
        //! The internal file storage path of the applet.
        AppletFiles,
        //! The path to the mass file storage for the applet.
        AppletMass,
        //! The path to the temporary file storage for the applet.
        AppletTemp,
        //! The storage path of the applet.
        AppletStorage,
        //! The logging directory of the RotateFileLoggerStream.
        LoggingDirectory,
        //! Allows user-defined roles up to UnknownRole.
        UserRole,
        UnknownRole = 16
    };

    class Entry {
    public:
        Entry() : m_permission() {}
        explicit Entry(const String &path, int permission = SystemReadWrite | UserRead);
        GX_NODISCARD bool isNull() const { return m_path.empty(); }
        GX_NODISCARD const String &path() const { return m_path; }
        GX_NODISCARD bool authorized(int permission) const {
            return (m_permission & permission) == permission;
        }

    private:
        String m_path;
        int m_permission;
    };
    typedef Vector<Entry> EntryList;

    /**
     * Gets the list of lookup paths for the resource package. Assuming the list is `["/system",
     * "/user"]`, then the package with URI `pkg://test.app` is found as follows:
     *     1. Open the `test.app.pkg` package in the `/user` directory
     *     2. Go to the `/system` directory and look for the `test.app.pkg` resource package
     * @see reset().
     * @note The return value is mutable, so you can append the resource package lookup path with
     * the EntryList::push_back() function. But this should only be done at initialization time.
     * @return A list of resource package lookup paths.
     */
    static EntryList &packages();
    /**
     * Gets the entry of the specified role.
     * @param role The role index.
     */
    static const Entry &entry(int role);
    /**
     * Sets the path entry for the specified role.
     * @param role The role index.
     * @param entry The entry object containing the route and selection information.
     */
    static void setEntry(int role, const Entry &entry);
    /**
     * Sets the path entry for the specified role.
     * @param role The role index.
     * @param path The path field of the Path entry.
     * @param permission The permission field of the Path entry.
     */
    static void setEntry(int role, const String &path, int permission = SystemReadWrite | UserRead);
    /**
     * Resets the state of all properties, including the global resource package path for the `file`
     * and `pkg` URI protocol, the resource lookup path list, etc. This function will be called when
     * the CoreApplication class is destructured.
     */
    static void reset();

private:
    class Private;
};

inline EnvPath::Entry::Entry(const String &path, int permission)
    : m_path(path), m_permission(permission) {}

inline void EnvPath::setEntry(int role, const String &path, int permission) {
    setEntry(role, Entry(path, permission));
}
} // namespace gx
