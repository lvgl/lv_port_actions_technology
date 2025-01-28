/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"

namespace gx {
class Widget;
class Texture;
class ByteArray;

class ImageWriter {
public:
    explicit ImageWriter(const Texture &texture) : m_texture(texture) {}
    bool savePNG(const String &path) const; // NOLINT(*-use-nodiscard)
    GX_NODISCARD ByteArray encodePNG() const;

private:
    const Texture &m_texture;
};

class WidgetImageWriter {
public:
    explicit WidgetImageWriter(Widget *widget) : m_widget(widget) {}
    bool savePNG(const String &path) const; // NOLINT(*-use-nodiscard)

private:
    Widget *m_widget;
};
} // namespace gx
