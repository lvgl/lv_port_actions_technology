/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"
#include "gx_eventloop.h"

namespace gx {
class String;
class Object;
class Event;
class EventDispatcher;
class PlatformAdapter;
class ResourceManager;
class FontManager;

/**
 * @brief The CoreApplication class provides the foundation of the Glyphix, include the event loop,
 * resource manager, font manager, and platform adapter, etc.
 *
 * To use Glyphix, you must create an object of this class. It is typically defined as a stack
 * variable within the UI thread and can be accessed via the \ref CoreApp().
 *
 * @see CoreApp()
 */
class CoreApplication {
public:
    explicit CoreApplication(PlatformAdapter *adapter = nullptr);
    virtual ~CoreApplication();

    /**
     * Returns whether the application has been quited.
     * @see quit()
     */
    GX_NODISCARD bool isQuited() const;
    GX_NODISCARD uint32_t currentTick() const;
    void installDispatcher(gx::EventDispatcher *dispatcher);
    /**
     * Sends an event to the event queue, which is later processed in processEvents() of the UI
     * thread.
     * @see processEvents()
     * @see sendEvent()
     */
    void postEvent(Event *event);
#ifndef GX_COPMATIBLE_CXX_98
    void postTask(std::function<void()> task);
#endif
    /**
     * Send an event to the Application and process it immediately.
     * @see postEvent()
     */
    void sendEvent(Event *event);
    virtual bool bubblingEvent(Object *target, Event *event);
    /**
     * Posts an asynchronous event, which is not handled in UI thread, but is forwarded to another
     * thread.
     * @param event The asynchronous event to be post.
     */
    void postAsyncEvent(Event *event);
#ifndef GX_COPMATIBLE_CXX_98
    void postAsyncTask(std::function<void()> task, int priority = TaskEvent::NormalPriority);
#endif
    /**
     * Set the asynchronous event proxy. If there is no proxy it will handle the event in the UI
     * thread (just like postEvent()).
     * @param proxy The proxy object forwards asynchronous events to other threads.
     */
    void setAsyncProxy(Object *proxy);
    //! Get the resource manager object.
    GX_NODISCARD ResourceManager *resourceManager() const;
    //! Get the font manager object.
    GX_NODISCARD FontManager *fontManager() const;
    /**
     * Processes the events in the event queue until the queue is empty.
     * @param filter event filter, only matching events will be processed.
     * @return Returns false if the application has exited, otherwise returns true.
     * @see EventLoop::EventFilter
     * @see postEvent()
     * @see quit()
     * @see exec()
     */
    bool processEvents(EventLoop::EventFilter filter = EventLoop::AllEvents);
    EventLoop *eventLoop();
    /**
     * Repeat processEvents() until the application quits.
     * @see quit()
     * @see processEvents()
     */
    int exec();
    /**
     * Send an quit event to the event queue to quit the application.
     * @see isQuited()
     */
    void quit();

    /**
     * Gets the runtime log filtering level with an initial value of GX_LOG_DEBUG (all logs are
     * output).
     * @see setLoggingLevel()
     */
    GX_NODISCARD static int loggingLevel();
    /**
     * Set the runtime log filtering level. But the log will never output if it is statically
     * filtered by GX_LOG_LEVEL.
     * @param level Logs smaller than the log filtering level will not be output. Take one of the
     * values from GX_LOG_DEBUG to GX_LOG_FATAL.
     * @see loggingLevel()
     */
    static void setLoggingLevel(int level);
    /**
     * An reference value used to guide developers in trimming features based on a predetermined
     * memory budget. It helps optimize and adjust the application's functionality for smooth
     * performance on low-end devices, regardless of the device's actual memory capacity.
     * @see setMemoryProfile()
     */
    static int memoryProfile();
    /**
     * Set the memory profile level.
     * @param level The memory profile level, the unit is KiB.
     * @see memoryProfile()
     */
    static void setMemoryProfile(int level);
    static String versionString();

    GX_NODISCARD void *thisThread();

protected:
    PlatformAdapter *platformAdapter() const;
    virtual void event(EventLoop::EventFilter filter);

private:
    struct Data;
    explicit CoreApplication(Data *d);
    GX_NODISCARD Data *impl() const { return m_d; }

private:
    Data *m_d;
    static CoreApplication *m_instance;
    friend class Application;
    friend class PrimitiveObject;
    friend CoreApplication *CoreApp(); // NOLINT(readability-*)
};

/**
 * @brief Get the global CoreApplication object.
 *
 * This function is a shortcut to access the global CoreApplication object. The nullptr will be
 * returned if the CoreApplication object has not been created.
 *
 * @see App()
 */
inline CoreApplication *CoreApp() { // NOLINT(readability-*)
    return CoreApplication::m_instance;
}
} // namespace gx
