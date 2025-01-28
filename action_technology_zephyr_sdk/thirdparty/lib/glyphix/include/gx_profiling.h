/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_geometry.h"
#include "gx_string.h"
#include "gx_vector.h"

namespace gx {
class Widget;
class PaintDevice;

namespace prof {
enum StringId {
    ID_DrawPoint,
    ID_DrawLine,
    ID_DrawRect,
    ID_FillRect,
    ID_DrawTexture,
    ID_DrawPath,
    ID_FillPath,
    ID_DrawText,
    ID_LoadImage,
    ID_LoadFont,
    ID_LoadGlyph,
    ID_LoadPage,
    ID_LoadUi,
    ID_RenderUi,
    ID_BuildComponent,
    ID_ImportScript,
    ID_LoadScript,
    ID_RunScript,
    ID_LifeCycle,
    ID_ScriptCall,
    ID_NativeCall,
    ID_LoadApp,
    ID_PreloadImage,
    ID_ScriptGC,
    ID_UserExtra = 30,
    ID_AutoAlloc = 80,
    ID_Unknown = 255
};

enum ThreadId { MainThread, RenderThread, AsyncThread };

enum TraceSignal { TS_VsyncSignal, TS_FlipScreenStart, TS_FlipScreenStop, TS_TouchEvent };

#ifdef GX_ENABLE_PROFILING
class Backend {
public:
    virtual void postFrame(const Vector<char> &frame) = 0;
    virtual void startLogging();
    virtual void stopLogging();
    GX_NODISCARD const Vector<char> &headFrame() const { return m_headFrame; }
    void setup();

    static void setLogging(bool enable);
    static Backend *instance();

protected:
    Backend();

private:
    Vector<char> m_headFrame;
};

class WidgetFrame {
public:
    explicit WidgetFrame(const Widget *widget);
    ~WidgetFrame();
    std::size_t revertOffset, continueOffset;
};

class WidgetPaint {
public:
    explicit WidgetPaint();
    ~WidgetPaint();
};

class DrawCall {
public:
    explicit DrawCall(int id, const Rect &rect = Rect());
    ~DrawCall();
};

class ResolveLayout {
public:
    explicit ResolveLayout(const Widget *widget);
    ~ResolveLayout();
};

class FlatGeneric {
public:
    explicit FlatGeneric(int id, const String &msg = String());
    ~FlatGeneric();
    static void record(int id, const String &msg, uint64_t start, ThreadId thread = MainThread);

private:
    int m_id;
    String m_msg;
    uint64_t m_timestamp;
};

class RecursionGeneric {
public:
    explicit RecursionGeneric(int id, const String &msg);
    RecursionGeneric(int id, const char *tag, const String &msg = String());
    ~RecursionGeneric();
};

class PausedInterval {
public:
    PausedInterval();
    ~PausedInterval();

private:
    uint64_t m_timestamp;
};

bool isEnabled();
void startRender(const PaintDevice *device);
void stopRender(const PaintDevice *device);
void traceSignal(TraceSignal tag);

#define GX_PROF_WIDGET_FRAME(...) prof::WidgetFrame pwf_(__VA_ARGS__)
#define GX_PROF_WIDGET_PAINT()    prof::WidgetPaint pwp_
#define GX_PROF_DRAW_CALL(...)    prof::DrawCall pdcall_(__VA_ARGS__)
#define GX_PROF_LAYOUT(...)       prof::ResolveLayout pdcall_(__VA_ARGS__)
#define GX_PROF_RESOURCE(...)     prof::FlatGeneric plres_(__VA_ARGS__)
#define GX_PROF_RESOURCE_REC(...) prof::RecursionGeneric plres_(__VA_ARGS__)
#else
inline void startRender(const PaintDevice *) {}
inline void stopRender(const PaintDevice *) {}
inline void traceSignal(TraceSignal) {}

#define GX_PROF_WIDGET_FRAME(...)
#define GX_PROF_WIDGET_PAINT(...)
#define GX_PROF_DRAW_CALL(...)
#define GX_PROF_LAYOUT(...)
#define GX_PROF_RESOURCE(...)
#define GX_PROF_RESOURCE_REC(...)
#endif
} // namespace prof
} // namespace gx
