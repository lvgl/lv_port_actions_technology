/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_appletobject.h"
#include "gx_signals.h"
#include "gx_transitionwidget.h"

namespace gx {
class Uri;
class AppletPage;
class Component;
class AbstractAnimation;
class AppletTimer;
class AppletTester;
class UIObject;
class FontFaceMap;

/**
 * Example of an applet in a multi-application framework. An applet is an GUI entity that can be
 * displayed, and applets manage their own pages and resources.
 */
class Applet : public TransitionWidget, public AppletResource {
    GX_OBJECT

public:
    //! The flags used to setFlags() and flags() in AppletPage.
    enum Flags {
        //! The visible status of the page has changed.
        VisibleUpdated = UserWidgetFlags,
        //! Mark the component being rendered, @see isRendering().
        RenderingComponents = UserWidgetFlags << 1,
        //! Mark the launcher app.
        Launcher = UserWidgetFlags << 2,
        //! The language has been updated.
        LanguageUpdated = UserWidgetFlags << 3,
        //! The applet is created by \ref AppletKit::launch().
        LaunchByAppletKit = UserWidgetFlags << 4,
        //! Mark the applet to keep alive.
        KeepAlive = UserWidgetFlags << 5
    };
    /**
     * The applet life-cycle tag for the @ref preLifeCycle signal. OnCreate is not provided because
     * the signal cannot be connected earlier than the applet is created.
     * @see preLifeCycle
     */
    enum LifeCycle {
        OnShow,   //!< The applet switches from background to foreground.
        OnHide,   //!< The applet switches from foreground to background.
        OnDestroy //!< The applet will be destructured.
    };

    class Manifest;
    class UriSolver;

    /**
     * The constructor performs the basic initialization of the applet, such as creating a separate
     * JavaScript runtime environment, setting the initial state, and so on. However, the
     * constructor does not execute the launch() function.
     * @param parent The parent widget of the applet, which will be displayed in this widget.
     * @param name The name of the applet, which is also a unique identifier.
     * @param package The resource package name of the applet, e.g. `app://clock`.
     */
    explicit Applet(Widget *parent = nullptr, const String &name = String(),
                    const String &package = String());
    virtual ~Applet();

    //! Check if the applet is the launcher.
    GX_NODISCARD bool isLauncher() const { return flags() & Launcher; }
    /**
     * Gets the JavaScript realm of the applet. Each realm has its own global object environment. So
     * each applet also has its own global object environment.
     * @note To load a script or create a function in the environment of a given applet, you need to
     * switch to the JavaScript realm of that applet first.
     */
    GX_NODISCARD const JsVM::Realm &realm() const;
    /**
     * Get the manifest data of the applet, which corresponds to the manifest.json file in the
     * applet resource package. For more information, please refer to the applet development
     * documentation.
     */
    GX_NODISCARD const Manifest &manifest() const;
    /**
     * Launch the applet. This mainly loads the manifest data, loads the app.js script, and calls
     * the `onCreate()` lifecycle function. Do not call this function repeatedly for applets
     * launched by the AppletKit::launch() function.
     * @see AppletKit::launch()
     */
    void launch();
    /**
     * Loads a page in the applet. The loaded page will not be mounted anywhere by default, instead
     * you need to deal with it.
     * @param name The name of the page, defined in manifest.json.
     * @param params Parameters specified when loading the page (optional). The parameters will be
     * merged into the view-model properties of the page component.
     * @return A pointer to the loaded page instance, or nullptr if the load fails.
     */
    GX_NODISCARD AppletPage *loadPage(const String &name, const JsValue &params = JsValue());
    /**
     * Load a page and push it into the top of the page stack of this applet. This function actually
     * calls loadPage(const String &, const JsValue &) first, and then pushPage(AppletPage *).
     * @param name The name of the page, defined in manifest.json.
     * @param params Parameters specified when loading the page (optional). The parameters will be
     * merged into the view-model properties of the page component.
     * @return Specifies whether the page was successfully loaded and pushed.
     * @see loadPage(const String &, const JsValue &)
     * @see pushPage(AppletPage *)
     */
    bool pushPage(const String &name, const JsValue &params = JsValue());
    /**
     * Pushes a page object to the top of this applet's page stack. The applet will maintain this
     * page instance, do not delete the page manually.
     * @param page Pointer to the page instance that needs to be pushed.
     * @return Specifies whether the page was successfully pushed. Fails only if \p page is nullptr.
     * @see pushPage(const String &, const JsValue &)
     */
    bool pushPage(AppletPage *page);
    /**
     * Load a page and replace it into the top of the page stack of this applet. This function
     * actually calls loadPage(const String &, const JsValue &) first, and then
     * replacePage(AppletPage *).
     * @param name The name of the page, defined in manifest.json.
     * @param params Parameters specified when loading the page (optional). The parameters will be
     * merged into the view-model properties of the page component.
     * @return Specifies whether the page was successfully loaded and replaced.
     * @see loadPage(const String &, const JsValue &)
     * @see replacePage(AppletPage *)
     */
    bool replacePage(const String &name, const JsValue &params = JsValue());
    /**
     * Replaces the top page with the specified page.
     * @param page The page object.
     * @return Whether the operation succeeded or not. Failed when \p page is nullptr or there is no
     * page in the applet.
     * @see replacePage(const String &, const JsValue &)
     */
    bool replacePage(AppletPage *page);
    /**
     * Load a pop-up window and display it. The page stack of the popups is not maintained
     * by the Applet.
     * @param name The name of the popup page, defined in manifest.json.
     * @param params Parameters specified when loading the page (optional). The parameters will be
     * merged into the view-model properties of the page component.
     * @return Specifies whether the popup page was successfully loaded and show.
     * @see loadPage(const String &, const JsValue &)
     * @see showPopup(AppletPage *)
     */
    bool showPopup(const String &name, const JsValue &params = JsValue());
    /**
     * Show a popup page.
     * @param page The pointer to the popup page.
     * @return Specifies whether the popup page was successfully loaded and show.
     * @see loadPage(const String &, const JsValue &)
     * @see showPopup(const String &, const JsValue &)
     */
    bool showPopup(AppletPage *page);
    /**
     * Returns to the specified page, and any other pages above that page will be removed from the
     * page stack and destroyed.
     * @param name The name of the page to be returned. If the page does not exist in the page
     * stack, no action will be performed.
     * @return Specifies whether a page is returned.
     */
    bool backPage(const String &name);
    /**
     * Returns to the previous page. The page at the top of the page stack will be pop and
     * destroyed.
     * @return Specifies whether a page is returned. Fails when the page stack is empty.
     */
    bool backPage();
    /**
     * Close and popup the page, and delete the page object.
     * @param page The instance of the page to be closed, which must exist in the page stack of the
     * applet.
     */
    void closePage(AppletPage *page);
    /**
     * Close and destroy all pages and the page stack will be emptied.
     */
    void clearPages();
    /**
     * Set the visibility of the applet. This function causes the `onShow()` or `onHide()` lifecycle
     * function of the applet to be called.
     */
    virtual void setVisible(bool state);
    /**
     * Activate this applet and play the transition animation if configured. The activated applet
     * will be located in the foreground and visible.
     *
     * Pop all applets above this applet in navigation stack if it's in the navigation stack.
     * @see inactivate()
     */
    void activate();
    /**
     * Inactivate this applet and play the transition animation if configured. Only activated
     * applets can be inactivated and pop the navigation stack.
     * @see activate()
     */
    void inactivate();
    /**
     * Get the page stack of the applet. The *back* is the bottom page and *top* is the top page.
     */
    GX_NODISCARD const Vector<AppletPage *> &pages() const;
    /**
     * Get the AppletObject object of the applet.
     * @see AppletObject
     */
    GX_NODISCARD AppletObject &model() { return m_model; }
    /**
     * Get the AppletObject object of the applet.
     * @see AppletObject
     */
    GX_NODISCARD const AppletObject &model() const { return m_model; }
    /**
     * Gets the JavaScript model object of the applet, which is exported from the app.js file of the
     * applet resource package.
     */
    GX_NODISCARD const JsValue &modelObject() const { return model().modelObject(); }
    /**
     * Get the name of the transition name defined in manifest.json for the specified page
     * object.
     * @param page The page to be queried.
     * @param mode Transition mode, the type is actually AppletKit::TransitionMode.
     * @return The name of the transition, otherwise returns empty if not configured.
     */
    GX_NODISCARD String transitionName(const AppletPage *page, int mode) const;
    GX_NODISCARD AppletTimer *createTimer();
    bool removeTimer(int id);
    /**
     * Get the number of timers running in the applet.
     */
    GX_NODISCARD int timerCount() const;
    /**
     * Bind an object to the applet. The bound objects are deleted when the applet is destructured.
     * @param object The object to be bound.
     * @return The identifier of the bound object (`id > 0` if valid).
     * @see unbindObject()
     */
    int bindObject(PrimitiveObject *object);
    /**
     * Unbind object from the applet. This function needs to be called when there is no need to bind
     * the object to the applet, or before manually deleting the object. We recommend using
     * unbindObject(int) for performance reasons.
     * @param object The object to be unbundled from the applet.
     * @return Returns true if the unbinding was successful.
     * @see unbindObject(int)
     */
    bool unbindObject(PrimitiveObject *object);
    /**
     * Unbind object from the applet. This function needs to be called when there is no need to bind
     * the object to the applet, or before manually deleting the object. This function has a
     * constant time complexity.
     * @param id The identifier of the bound object, which is returned by the bindObject().
     * @param metaObject Unbinding occurs only on the specified non-null meta-object type.
     * @return Unbound object, or nullptr if there is no action.
     * @see bindObject()
     */
    PrimitiveObject *unbindObject(int id, const MetaObject *metaObject = nullptr);
    /**
     * Unbind object from the applet. This function needs to be called when there is no need to bind
     * the object to the applet, or before manually deleting the object. This function has a
     * constant time complexity.
     * @tparam T The unbind the object type, and do nothing if the object type is not compatible.
     * @param id The identifier of the bound object, which is returned by the bindObject().
     * @return Unbound object, or nullptr if there is no action.
     * @see bindObject()
     */
    template<class T> T *unbindObject(int id);
    /**
     * Renders all pages in the applet page stack, excluding popup pages. The rendering operation
     * construct the component tree from the associated UI object resource file (*.uo). The popup
     * page is rendered by the AppletKit class.
     * @see isRendering()
     */
    void render();
    bool postRender();
    GX_NODISCARD const FontFaceMap &fontFaces() const;
    /**
     * Get whether the current applet is executing rendering. This flag contains only the component
     * tree generation when the component is rendered, and the flag is cleared temporarily in the
     * lifecycle function calls during this period.
     * @return Return true when the applet's page (includes pages in the page stack and popups) is
     * rendering.
     * @see render()
     */
    GX_NODISCARD bool isRendering() const { return flags() & RenderingComponents; }
    /**
     * Open a UI object file and read the cache directly if the UI object is already in the cache.
     * This function is called when the component is constructed to load a UIObject instance.
     * @param uri URI of the UI object (`*.uo`) resource file.
     * @return Loaded UI object object. If the load fails a null reference will be returned.
     */
    GX_NODISCARD virtual SharedRef<UIObject> openUIObject(const String &uri);
    virtual void flush(const Uri &pattern, bool strict);
    /**
     * Resolve URI to native level. The specific rules are.
     *   1. The path `/path` is translated to `app://app-name/path`.
     *   2. The `internal` URI is translated to the specific resource corresponding to this applet.
     *      The `internal` protocol is generally translated to paths to facilitate direct
     *      manipulation with the `Dir` class or the file system API.
     *   3. Other URIs are returned directly.
     * @param uri The URI to be translated.
     * @return The translated URI.
     */
    GX_NODISCARD String resolveUri(const String &uri);
    /**
     * Import a JavaScript module into the environment of the current applet. This function has no
     * permission restrictions.
     * @param uri The URI of the module to be import. Paths and internal URIs are not supported.
     * @return The value of the module after importing.
     */
    GX_NODISCARD JsValue require(const String &uri);
    /**
     * Get the permission of the applet.
     * @see EnvPath::Permission
     * @see setPermission()
     */
    GX_NODISCARD int permission() const;
    /**
     * Set the permission for the applet.
     * @see EnvPath::Permission
     * @see permission()
     */
    void setPermission(int permission);
    //! Get the applet language name.
    GX_NODISCARD String language() const;
    //! Set the applet language name. This function will reset the i18n resource file.
    void setLanguage(const String &name);
    //! Get the translated i18n text via key.
    GX_NODISCARD String translate(const String &key);
    AppletTester *tester();

    GX_NODISCARD virtual int objectKinds(int hint) const;

    /**
     * Gets the applet instance from the current JavaScript context. As described in realm(), each
     * applet has a unique JavaScript runtime environment, so in turn the former can be accessed
     * through the latter.
     * @param vm The specified JsVM instance, usually by \ref JsVM::current().
     * @return The applet instance from the current JavaScript context. A nullptr is returned if the
     * JavaScript does not run in the realm of any applet.
     * @note If you want to get the applet that the current JavaScript code is in, then you have to
     * make sure that the JavaScript runtime is executing the code in the applet at that moment
     * (e.g. callback functions in the GUI, scripts loaded in the applet, etc.), otherwise you will
     * just get nullptr.
     */
    GX_NODISCARD static Applet *current(const JsVM &vm);
    GX_DEPRECATED_MSG("use JsVM::current(const JsVM &) instead")
    GX_NODISCARD static Applet *current(const JsVM *vm) { return current(*vm); }

    /**
     * Emits this signal before calling the JavaScript life-cycle function. The argument of the
     * signal is actually a @ref LifeCycle value.
     * @see LifeCycle
     */
    Signal<int> preLifeCycle;

protected:
    bool layoutEvent(LayoutEvent *);
    GX_NODISCARD virtual AbstractAnimation *transition(const String &name, int mode);

private:
    class Private;
    Private *d;
    AppletObject m_model;
    friend class AppletKit;
    friend class AppletTimer;
};

class Applet::UriSolver {
public:
    explicit UriSolver(Applet *applet) : m_applet(applet) {}
    String operator()(const String &uri) const { return m_applet->resolveUri(uri); }

private:
    Applet *m_applet;
};

inline bool Applet::pushPage(const String &name, const JsValue &params) {
    return pushPage(loadPage(name, params));
}

inline bool Applet::replacePage(const String &name, const JsValue &params) {
    return replacePage(loadPage(name, params));
}

inline bool Applet::showPopup(const String &name, const JsValue &params) {
    return showPopup(loadPage(name, params));
}

template<class T> T *Applet::unbindObject(int id) {
    return dyn_cast<T *>(unbindObject(id, &T::staticMetaObject));
}
} // namespace gx
