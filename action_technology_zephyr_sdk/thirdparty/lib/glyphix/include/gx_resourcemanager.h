/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"
#include "gx_uri.h"

namespace gx {
class ResourceFile;
class ResourceInterface;
class Mutex;

/**
 * The ResourceManager class is the core component of the Resource Framework. This
 * class provides some important core interfaces for use by the other components of the RMF.
 */
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    /**
     * Install the specified Resource Interface Protocol. This operation should only be performed
     * during the initialisation phase of the resource management framework (otherwise the behaviour
     * would be unpredictable). A better place for this is in the \ref GX_CORE_MODULES_MOUNT
     * block, which will be executed when the \ref CoreApplication class is constructed.
     * @param protocol The name of the Resource Interface Protocol.
     * @param interface A pointer to an instance of the Resource Interface Protocol.
     * @return Returns true if the installation was successful, otherwise false`.
     * @note The object pointed to by the \p interface must be constructed by new, after which the
     * ResourceManager will manage it.
     */
    bool install(const char *protocol, ResourceInterface *interface);
    /**
     * Uninstall the specified resource interface protocol. Do not uninstall the protocol while the
     * Resource Management Framework is running.
     * @param protocol The name of the Resource Interface Protocol.
     * @return Returns true if the uninstallation was successful, otherwise false.
     */
    bool uninstall(const char *protocol);
    GX_NODISCARD ResourceInterface *interface(const char *protocol) const;
    /**
     * When a resource package is refreshed, the resource cache associated with this package is
     * cleared. This function needs to be called before updating or deleting a resource package.
     * The specific resource protocol defines the resource refresh operation by
     * ResourceInterface::refresh(). For example, The PRC protocol flushes the internal cache and
     * closes PRC file handle. The next update or delete step (e.g. deleting a resource package
     * file) must be executed immediately to avoid a new resource operation causing the resource
     * package to be reloaded.
     *
     * This function also emits the \ref refreshSignal signal to other modules that are interested
     * in resource refresh.
     * @see refreshSignal
     * @see ResourceInterface::refresh()
     * @param package The URI of the resource package.  Only the scheme and authority fields (i.e.,
     * protocol and package name) are available.
     * @return Whether the resource package was refreshed successfully (it will not be refreshed if
     * the resource package does not exist).
     */
    bool refresh(const String &package);
    /**
     * This function just returns the value of resolve(uri, uri->uri()).
     * \see resolve(Uri *, const String &) const
     */
    GX_NODISCARD bool resolve(Uri *uri) const { return resolve(uri, uri->uri()); }
    /**
     * Resolve a URI into the original URI of the resource, which can then be located directly to
     * the resource package and open the resource file.
     * @param uri A pointer to the URI object to be resolved. After the translation operation the
     * object is modified to the original URI of the resource.
     * @param origin The string of the URI to be resolved.
     * @return Return true if the resource exists and was resolved successfully, otherwise
     * false.
     * @note This function is not thread-safe and need take mutex() if it is not used in UI thread.
     */
    bool resolve(Uri *uri, const String &origin) const;
    /**
     * Open the specified resource file.
     * @param uri The URI object assigned to the resource file.
     * @param flags The open flags of the file.
     * @return If the file is opened, a pointer to the ResourceFile object is returned (this object
     * is destroyed by delete). Otherwise nullptr is returned.
     * @note This function is not thread-safe and need take mutex() if it is not used in UI thread.
     * In general, please use the \ref File class.
     */
    GX_NODISCARD ResourceFile *open(const Uri &uri, int flags) const;
    /**
     * Refreshes the resolve cache for the specified URI. The URI Translation Mapping
     * Cache needs to be flushed to regenerate the correct mapping cache when creating new files
     * etc.
     * @param uri The URI of the translation cache needs to be refreshed.
     * @note This function is not thread-safe and need take mutex() if it is not used in UI thread.
     */
    void flushResolveCache(const String &uri);
    /**
     * Flush the resolve and domain cache. This function will reset the Resource Manager cache and
     * free memory. This function is thread-safe, because it is take mutex() automatically.
     */
    void flushCache();
    /**
     * Get the resource manager's mutex. This mutex exclusion safeguards multi-threaded resource
     * operations (e.g. file IO).
     * \return Reference to the Resource Manager mutex.
     */
    GX_NODISCARD Mutex &mutex() const;

    /**
     * Resolve the URI to a path in the file system. Only paths and global file protocols can be
     * resolved, but not in other cases. For example, `/foo/baz` will be resolved as `/foo/baz`;
     * and `file:///foo/baz` will be resolved to `/foo/baz` (without global resource package).
     * This function does not check the existence of the path.
     * @param uri The URI or path string to be resolved.
     * @return The path to the file system where the URI is mapped. An empty string will be returned
     * if the resolve fails.
     */
    GX_NODISCARD static String resolvePath(const String &uri);

    /**
     * Triggered when the refresh() is called. Other modules can connect to this signal to detect
     * resource packet refreshes.
     * \param Uri The Uri object of the resource being refreshed.
     */
    Signal<Uri> refreshSignal;

private:
    class Private;
    Private *m_d;
};
} // namespace gx
