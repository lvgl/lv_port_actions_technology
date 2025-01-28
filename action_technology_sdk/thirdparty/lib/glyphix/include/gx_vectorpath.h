/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_geometry.h"
#include "gx_string.h"
#include "gx_vector.h"

namespace gx {
class Pen;
class Font;
class Transform;

class VectorPath {
public:
    enum VertexType { MoveToVertex, LineToVertex, CubicToVertex };

    enum Alignment { AlignStart = 0, AlignEnd, AlignMiddle };

    struct Vertex {
        float x, y;
        VertexType type;
        GX_NODISCARD bool isMoveTo() const { return type == MoveToVertex; }
        GX_NODISCARD bool isLineTo() const { return type == LineToVertex; }
        GX_NODISCARD bool isCurveTo() const { return type == CubicToVertex; }

        operator PointF() const { return PointF(x, y); }
        Vertex(const PointF &vertex, VertexType t) {
            x = vertex.x();
            y = vertex.y();
            type = t;
        }
        bool operator==(const Vertex &e) const { return fuzzyEqual(x, e.x) && fuzzyEqual(y, e.y); }
        bool operator!=(const Vertex &e) const { return !operator==(e); }
    };

    VectorPath();
    GX_NODISCARD bool isEmpty() const;
    void clear();
    void append(const VectorPath &path);
    void append(const Vertex &node) { m_path.push_back(node); }
    void moveTo(const PointF &point);
    void moveTo(float x, float y) { moveTo(PointF(x, y)); }
    void lineTo(const PointF &point);
    void lineTo(float x, float y) { lineTo(PointF(x, y)); }
    void conicTo(const PointF &c, const PointF &end);
    void conicTo(float x1, float y1, float x2, float y2);
    void cubicTo(const PointF &control1, const PointF &control2, const PointF &end);
    void cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
    void arcTo(const PointF &center, float radiusX, float radiusY, float startAngle,
               float stopAngle);
    void addRect(PointF topLeft, PointF bottomRight);
    void addRect(float x, float y, float width, float height);
    void addEllipse(float cx, float cy, float radiusX, float radiusY);
    void addText(float x, float y, const Font &font, const String &text);
    void addText(const PointF &point, const Font &font, const String &text);
    void arcText(const Point &center, float radius, float angle, const Font &font,
                 const String &text, Alignment align = AlignStart);
    void translate(float tx, float ty);
    void translate(const PointF &offset) { translate(offset.x(), offset.y()); }
    void transform(const Transform &transform);
    GX_NODISCARD VectorPath outline(const Pen &pen) const;
    GX_NODISCARD const Vector<Vertex> &path() const { return m_path; }
    void swap(VectorPath &path) { m_path.swap(path.m_path); }

    bool operator==(const VectorPath &other) const { return m_path == other.m_path; }
    bool operator!=(const VectorPath &other) const { return !(m_path == other.m_path); }

private:
    void firstInit();

private:
    Vector<Vertex> m_path;
};

inline void VectorPath::conicTo(float x1, float y1, float x2, float y2) {
    conicTo(PointF(x1, y1), PointF(x2, y2));
}

inline void VectorPath::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
    cubicTo(PointF(x1, y1), PointF(x2, y2), PointF(x3, y3));
}

inline void VectorPath::addEllipse(float cx, float cy, float radiusX, float radiusY) {
    arcTo(PointF(cx, cy), radiusX, radiusY, 0, 360);
}

inline void VectorPath::addText(const PointF &point, const Font &font, const String &text) {
    addText(point.x(), point.y(), font, text);
}

const Logger &operator<<(const Logger &, const VectorPath &);
} // namespace gx
