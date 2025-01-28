/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_applet.h"

namespace gx {
class AppletTester : public PrimitiveObject {
    GX_OBJECT

public:
    typedef Vector<std::pair<String, JsValue> > TestCaseList;

    explicit AppletTester(Applet *applet);
    virtual ~AppletTester();
    GX_NODISCARD const JsValue &locatorPrototype() const { return m_locatorPrototype; }
    GX_NODISCARD Applet *applet() const { return m_applet; }
    GX_NODISCARD const TestCaseList &testcases() const { return m_testcases; }
    GX_NODISCARD bool isRunning() const { return !m_running.empty(); }
    GX_NODISCARD const String &runningTestcase() const { return m_running; }
    void pushTestcase(const String &name, const JsValue &value);
    GX_NODISCARD bool isMounted() const { return m_mounted; }
    GX_NODISCARD bool isTerminated() const { return m_terminate; }
    JsValue startTest(const String &name);
    void terminate() { m_terminate = true; }

private:
    struct Private;
    uint32_t m_mounted : 1;
    uint32_t m_terminate : 1;
    Applet *m_applet;
    JsValue m_locatorPrototype;
    TestCaseList m_testcases;
    String m_running;
};
} // namespace gx
