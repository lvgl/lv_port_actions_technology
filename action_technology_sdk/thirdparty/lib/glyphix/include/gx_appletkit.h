/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"
#include "gx_signals.h"

namespace gx {
class Uri;
class Applet;
class PackageDatabase;
class BriefManifest;
class AbstractAnimation;
class Transitions;
class Widget;

class AppletKit : NonCopyable<AppletKit> {
public:
    enum Options {
        //! There are no applet used as launcher, so all applets can be inactivated or exited.
        NoLauncherApplet = 1 << 0
    };
    //! The page and applet a transition mode.
    enum TransitionMode { OpenEnter = 0, CloseEnter = 1, OpenExit = 2, CloseExit = 3 };
    enum PackageStatus {
        ValidPackage = 0,
        InvalidDevice,
        InvalidPackage,
        InvalidVersion,
        InvalidSignature,
        FileNotExists,
        FileIOError,
        UnknownError,
    };
    enum PackageVerify { NormalVerify = 0, UpgradeOnly, IgnoreVersion };

    explicit AppletKit(Widget *window, const String &database = String());
    AppletKit(Widget *window, PackageDatabase *database);
    ~AppletKit();

    //! Get the parent window widget of the applet.
    GX_NODISCARD Widget *appletsWindow() const;
    //! Get the parent window widget of the toast.
    GX_NODISCARD Widget *toastsWindow() const;
    //! Get the parent window widget of the popup.
    GX_NODISCARD Widget *popupsWindow() const;
    //! Get the active applet (top of the applet stack), also used to check if there is an active
    //! applets.
    GX_NODISCARD Applet *activeApplet() const;
    //! Get the applet stack.
    GX_NODISCARD const Vector<Applet *> &appletStack() const;
    /**
     * Launch an applet as foreground, create and cache the applet if it is not cached.
     * @param name The name (identifier) of the applet.
     * @return The launched applet, and return nullptr if failed.
     * @see appletCached
     */
    Applet *launch(const String &name);
    Applet *launch(const Uri &uri);
    /**
     * Cache an applet directly, do nothing for cached applets.
     * @param applet The applet to be cached, and the key is applet->objectName(). The applet must
     * be allocate by new.
     * @return Return true if a new applet is cached.
     * @see appletCached
     */
    bool cache(Applet *applet);
    /**
     * Lookup the applet with the specified package name.
     * @param package The applet's package name (defined by the \p package field of manifest.json).
     * @return Return applet object if the applet exists and has been launched, otherwise
     * nullptr.
     */
    GX_NODISCARD Applet *lookup(const String &package) const;
    //! Get the list of running apps.
    Vector<Applet *> appletInstances() const;
    GX_NODISCARD PackageDatabase *database(int table = 0);
    GX_NODISCARD int options() const;
    void setOptions(int options, bool enabled = true);
    void installWidget(const MetaObject *metaObject);
    void installWidget(const MetaObject *metaObject, const String &name);
    template<typename T> void installWidget();
    template<typename T> void installWidget(const String &name);
    GX_NODISCARD const MetaObject *findMetaObject(const String &name);
    //! Get the default key event handler object.
    GX_NODISCARD Object *defaultKeyHandler() const;
    /**
     * Set the default key event handler object. This object receives key events that are not
     * accepted.
     * @param object The default key event handler object.
     */
    void setDefaultKeyHandler(Object *object);
    GX_NODISCARD bool isTransitionPlay() const;
    GX_NODISCARD AbstractAnimation *transition(Widget *target, const String &name, int mode) const;
    GX_NODISCARD const String &defaultTransition(int mode) const;
    bool installTransitions(const String &name, Transitions *factory);
    bool uninstallTransitions(const String &name);
    void setDefaultTransition(TransitionMode mode, const String &animation);

    PackageStatus verifyPackage(const String &fileUri, PackageVerify policy = NormalVerify);
    /**
     * Install or update a package from the package file.
     * @param fileUri The URI of the package file.
     * @param policy The package verification policy.
     * @return The installation status. If the installation is successful, it returns ValidPackage.
     */
    PackageStatus installPackage(const String &fileUri, PackageVerify policy = NormalVerify);
    //! Uninstall the \p package specified by the package name.
    bool removePackage(const String &package);
    //! Get a list of installed applets, not including dials.
    GX_NODISCARD Vector<String> installedApplets() const;
    //! Get a list of installed dials, not including applets.
    GX_NODISCARD Vector<String> installedDials() const;
    //! Get the name and icon URI of the application.
    GX_NODISCARD BriefManifest queryPackage(const String &package) const;

    /**
     * Show a toast prompt box.
     * @param message The prompt message text.
     * @param duration The duration of toast show, in milliseconds.
     */
    void showToast(const String &message, int duration = 1000);

    //! Get the URI of the global component by name.
    GX_NODISCARD String globalComponent(const String &name) const;
    /**
     * Registers a global component.
     * @param name The name of the global component.
     * @param uri URI of the global component UIObject file.
     */
    void setGlobalComponent(const String &name, const String &uri);
    //! Get the global language name.
    GX_NODISCARD String language() const;
    //! Set the global language name.
    void setLanguage(const String &name);

    GX_NODISCARD static AppletKit *instance() { return m_instance; }

    GX_NODISCARD static uint32_t vendorProductId();
    static void setVendorProductId(int vendorId, int productId);
    static const char *packageStatusMessage(PackageStatus status);

    /**
     * This signal is emitted when a new applet is cached.
     * @param 1 The target Applet object.
     * @see cache()
     * @see launch()
     */
    Signal<Applet *> appletCached;
    /**
     * This signal is emitted when the applet is activated or inactivated.
     * @param 1 The target Applet object.
     * @param 2 If true, the applet becomes active, otherwise it becomes inactive.
     */
    Signal<Applet *, bool> appletActiveChanged;
    Signal<String, bool> packageChanged;
    Signal<> beforeRender;
    Signal<> postRender;

private:
    class Private;
    Private *d;
    static AppletKit *m_instance;
    friend class Applet;
    friend class PageStack;
    friend class NativeComponent;
};

template<typename T> void AppletKit::installWidget() { installWidget(&T::staticMetaObject); }

template<typename T> void AppletKit::installWidget(const String &name) {
    installWidget(&T::staticMetaObject, name);
}
} // namespace gx
