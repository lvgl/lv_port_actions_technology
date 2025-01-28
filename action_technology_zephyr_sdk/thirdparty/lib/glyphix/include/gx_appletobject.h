/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"
#include "gx_object.h"

namespace gx {
class Applet;

class AppletResource {
public:
    GX_NODISCARD const String &resourceUri() const { return m_resourceUri; }
    void setResourceUri(const String &uri) { m_resourceUri = uri; }

private:
    String m_resourceUri;
};

class AppletObject {
public:
    typedef std::pair<JsValue, String> Result;
    explicit AppletObject(Applet *applet = nullptr);
    virtual ~AppletObject();

    GX_NODISCARD Applet *applet() const { return m_applet; }
    GX_NODISCARD const JsValue &modelObject() const { return m_modelObject; }
    void setModelObject(const JsValue &object) { m_modelObject = object; }
    /**
     * Call the object's lifecycle function.
     * @param source Source file information for the object.
     * @param name The name of the lifecycle function.
     */
    void callLifeCycle(const String &source, const JsValue &name) {
        callLifeCycle(source, name, nullptr, 0);
    }
    /**
     * Call the object's lifecycle function.
     * @param source Source file information for the object.
     * @param name The name of the lifecycle function.
     * @param args Arguments to pass to the function.
     * @param argc Number of arguments.
     */
    void callLifeCycle(const String &source, const JsValue &name, const JsValue *args, int argc);

private:
    Applet *m_applet;
    JsValue m_modelObject;
};
} // namespace gx
