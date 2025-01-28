/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_object.h"

namespace gx {
class Applet;

class BindableObject : public PrimitiveObject {
    GX_OBJECT

public:
    BindableObject() : m_applet(), m_bindId() {}
    virtual ~BindableObject();
    Applet *applet() const { return m_applet; }
    bool unbindApplet();

protected:
    virtual void unbind();

private:
    Applet *m_applet;
    int m_bindId;
    friend class Applet;
};
} // namespace gx
