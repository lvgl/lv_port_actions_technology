/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
class QRCode : public Widget {
    GX_OBJECT

public:
    explicit QRCode(Widget *parent = nullptr);

    GX_NODISCARD int version() const { return m_version; }
    GX_NODISCARD const String &value() const { return m_value; }
    void setValue(const String &value);

    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int width) const;

    GX_PROPERTY(String value, set setValue, get value)

protected:
    virtual void paintEvent(PaintEvent *event);

private:
    String m_value;
    Vector<uint8_t> m_qrcode;
    uint8_t m_size, m_version, m_eclevel;
};
} // namespace gx
