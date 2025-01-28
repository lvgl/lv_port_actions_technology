/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_coreapplication.h"
#include "gx_signals.h"

namespace gx {
class Font;
class Widget;
class EventLoop;
class PaintDevice;
class WidgetManager;
class StyleEngine;
class FontFaceMap;
struct PaintDeviceMetric;

/**
 * The Application class extends CoreApplication to provide platform-specific
 * functionalities, including managing the window, style engine, and various caches.
 * It also handles default font and pixel format settings.
 *
 * @see App()
 */
class Application : public CoreApplication {
public:
    //! These cache types are used to manage the caches in the application.
    enum CacheTypes {
        ImageTextureCache,   //!< Image and texture cache type.
        WidgetSnapshotCache, //!< Widget snapshot cache type.
        FontGlyphCache,      //!< Font glyph cache type.
    };

    /**
     * Constructs an Application object.
     * @param adapter Pointer to the platform adapter, defaults to nullptr.
     */
    explicit Application(PlatformAdapter *adapter = nullptr);
    virtual ~Application();

    //! Get the PaintDevice object of the screen.
    GX_NODISCARD PaintDevice *screenDevice() const;
    //! Get the metrics of the screen device.
    GX_NODISCARD const PaintDeviceMetric &screenMetric() const;
    //! Updates the screen metrics. This function is typically called when the screen size changes.
    void updateScreenMetric();
    //! Get the current window Widget of the application.
    GX_NODISCARD Widget *window() const;
    //! Set the main window of the application.
    void setWindow(Widget *window);
    //! Get the style engine used by the application.
    GX_NODISCARD StyleEngine *styleEngine() const { return m_styleEngine; }
    //! Set the style engine to be used by the application.
    void setStyleEngine(StyleEngine *styleEngine);
    //! Get the global default font.
    GX_NODISCARD const Font &font() const;
    //! Set the global default font. This function will refresh the font for all Widgets.
    void setFont(const Font &font);
    /**
     * Set the default pixel format for snapshots etc.
     * @param format The pixel format to use as default.
     * @param transparent Specifies the format is used for transparent snapshots.
     */
    void setDefaultPixelFormat(int format, bool transparent = false);
    /**
     * Get the default pixel format for snapshot etc.
     * @param transparent Should return the default format for transparent snapshots.
     */
    GX_NODISCARD int defaultPixelFormat(bool transparent = false) const;
    //! Get the current size of the specified cache type.
    GX_NODISCARD std::size_t cacheSize(CacheTypes category) const;
    //! Get the current capacity of the specified cache type.
    GX_NODISCARD std::size_t cacheCapacity(CacheTypes category) const;
    //! Set the capacity of the specified cache category.
    void setCacheCapacity(CacheTypes category, std::size_t capacity);
    /**
     * Flushes the specified number of bytes from the caches. This function is atomic and can be
     * called in case of memory allocation failure to release memory safely.
     * @param size The size of bytes to flush.
     * @return The size of bytes actually flushed, not necessarily equal to the requested size.
     */
    std::size_t flushCache(std::size_t size);
    //! Get the widget manager object.
    GX_NODISCARD WidgetManager *widgetManager() const;

    Signal<uint32_t> preTick;  //!< Signals emitted before each tick.
    Signal<uint32_t> postTick; //!< Signals emitted post each tick.

    struct Private;

protected:
    virtual void event(EventLoop::EventFilter filter);

private:
    class EventDispatcher;
    StyleEngine *m_styleEngine;
};

/**
 * @brief Get the global Application object.
 *
 * This function is a shortcut to access the global Application object. It gets the Application
 * object from the CoreApp() value by static cast.
 *
 * @warning If only a CoreApplication object exists without an Application object, this function
 * will result in a undefined behavior. This function is not thread-safe.
 *
 * @see CoreApp()
 */
inline Application *App() { // NOLINT(readability-*)
    return static_cast<Application *>(CoreApp());
}
} // namespace gx
