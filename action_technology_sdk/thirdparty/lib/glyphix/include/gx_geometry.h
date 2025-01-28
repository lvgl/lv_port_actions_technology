/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
class Size {
public:
    Size() : m_width(), m_height() {}
    Size(int width, int height);

    GX_NODISCARD int width() const { return m_width; }
    GX_NODISCARD int height() const { return m_height; }
    GX_NODISCARD bool isEmpty() const;

    void setWidth(int width);
    void setHeight(int height);
    void setSize(int width, int height);
    void reset();

    GX_NODISCARD Size intersected(const Size &size) const;
    GX_NODISCARD Size united(const Size &size) const;

    bool operator==(const Size &other) const;
    bool operator!=(const Size &other) const;
    Size operator+(const Size &other) const;
    Size operator-(const Size &other) const;
    Size operator&(const Size &other) const;
    Size operator|(const Size &other) const;
    Size &operator+=(const Size &other);
    Size &operator-=(const Size &other);
    Size &operator&=(const Size &other);
    Size &operator|=(const Size &other);

private:
    int m_width, m_height;
};

class Point {
public:
    Point();
    Point(int x, int y);

    GX_NODISCARD bool isEmpty() const { return !(m_x || m_y); }
    GX_NODISCARD int x() const { return m_x; }
    GX_NODISCARD int y() const { return m_y; }
    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }
    void setPoint(int x, int y);
    void translate(int dx, int dy);
    void translate(const Point &offset);
    void reset();
    GX_NODISCARD int modules() const;

    bool operator==(const Point &other) const;
    bool operator!=(const Point &other) const;
    Point operator+(const Point &other) const;
    Point operator-(const Point &other) const;
    Point operator-() const;
    Point &operator+=(const Point &other);
    Point &operator-=(const Point &other);

private:
    int m_x, m_y;
};

class PointF {
public:
    PointF() : m_x(0), m_y(0) {}
    PointF(float x, float y) : m_x(x), m_y(y) {}
    PointF(const Point &point);

    GX_NODISCARD bool isEmpty() const { return fuzzyZero(m_x) && fuzzyZero(m_y); }
    GX_NODISCARD float x() const { return m_x; }
    GX_NODISCARD float y() const { return m_y; }
    void setX(float x) { m_x = x; }
    void setY(float y) { m_y = y; }
    void setPoint(float x, float y) { setX(x), setY(y); }
    void translate(float dx, float dy);
    void translate(const PointF &offset);
    void reset();
    GX_NODISCARD float modules() const;
    GX_NODISCARD float angle() const;
    GX_NODISCARD float angleTo(const PointF &p) const;
    GX_NODISCARD float dotProduct(const PointF &b) const;
    GX_NODISCARD float crossProduct(const PointF &b) const;
    GX_NODISCARD bool isAcute(const PointF &b) const;

    PointF &operator+=(const PointF &other);
    PointF &operator-=(const PointF &other);
    PointF &operator*=(float scale);
    PointF &operator/=(float diver);
    operator Point() const;

private:
    float m_x, m_y;
};

class Rect {
public:
    Rect();
    Rect(int x, int y, int width, int height);
    Rect(const Point &point, const Size &size);
    Rect(const Point &topLeft, const Point &bottomRight);

    GX_NODISCARD int x() const { return m_x0; }
    GX_NODISCARD int y() const { return m_y0; }
    GX_NODISCARD int width() const { return m_x1 - m_x0 + 1; }
    GX_NODISCARD int height() const { return m_y1 - m_y0 + 1; }
    GX_NODISCARD int left() const { return m_x0; }
    GX_NODISCARD int top() const { return m_y0; }
    GX_NODISCARD int right() const { return m_x1; }
    GX_NODISCARD int bottom() const { return m_y1; }

    GX_NODISCARD Size size() const;
    GX_NODISCARD Point center() const;
    GX_NODISCARD PointF centerF() const;
    GX_NODISCARD Rect center(const Size &size) const;
    GX_NODISCARD Rect center(const Rect &rect) const { return center(rect.size()); }
    GX_NODISCARD Rect aligned(const Size &size, int align) const;
    GX_NODISCARD Rect aligned(const Rect &rect, int align) const {
        return aligned(rect.size(), align);
    }
    GX_NODISCARD Rect normalized() const;
    GX_NODISCARD Point topLeft() const { return Point(m_x0, m_y0); }
    GX_NODISCARD Point topRight() const { return Point(m_x1, m_y0); }
    GX_NODISCARD Point bottomLeft() const { return Point(m_x0, m_y1); }
    GX_NODISCARD Point bottomRight() const { return Point(m_x1, m_y1); }

    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD bool contains(int x, int y) const;
    GX_NODISCARD bool contains(const Point &point) const;
    GX_NODISCARD bool intersects(const Rect &rect) const;

    void clear();
    void setX(int x) { m_x1 += x - m_x0, m_x0 = x; }
    void setY(int y) { m_y1 += y - m_y0, m_y0 = y; }
    void setWidth(int width);
    void setHeight(int height);
    void setCoords(int x1, int y1, int x2, int y2);
    void setSize(int width, int height);
    void setSize(const Size &size);
    void setTop(int top) { m_y0 = top; }
    void setLeft(int left) { m_x0 = left; }
    void setBottom(int bottom) { m_y1 = bottom; }
    void setRight(int right) { m_x1 = right; }
    void moveTo(int x, int y);
    void moveTo(const Point &position);
    void move(const Point &offset);
    void move(int dx, int dy);
    void translate(const Point &offset) { move(offset); }
    void translate(int dx, int dy) { move(dx, dy); }
    void reset();

    GX_NODISCARD Rect intersected(const Rect &rect) const;
    GX_NODISCARD Rect united(const Rect &rect) const;

    bool operator==(const Rect &other) const;
    bool operator!=(const Rect &other) const;
    Rect operator&(const Rect &other) const;
    Rect operator|(const Rect &other) const;
    Rect operator+(const Point &offset) const;
    Rect operator-(const Point &offset) const;
    Rect &operator&=(const Rect &other);
    Rect &operator|=(const Rect &other);
    Rect &operator+=(const Point &offset);
    Rect &operator-=(const Point &offset);

private:
    int m_x0, m_y0, m_x1, m_y1;
};

class RectF {
public:
    RectF();
    RectF(float x, float y, float width, float height);
    RectF(const PointF &topLeft, const PointF &bottomRight);
    RectF(const Rect &rect);
    GX_NODISCARD float x() const { return m_x0; }
    GX_NODISCARD float y() const { return m_y0; }
    GX_NODISCARD float width() const { return m_x1 - m_x0; }
    GX_NODISCARD float height() const { return m_y1 - m_y0; }
    GX_NODISCARD float left() const { return m_x0; }
    GX_NODISCARD float top() const { return m_y0; }
    GX_NODISCARD float right() const { return m_x1; }
    GX_NODISCARD float bottom() const { return m_y1; }
    GX_NODISCARD PointF topLeft() const { return PointF(m_x0, m_y0); }
    GX_NODISCARD PointF topRight() const { return PointF(m_x1, m_y0); }
    GX_NODISCARD PointF bottomLeft() const { return PointF(m_x0, m_y1); }
    GX_NODISCARD PointF bottomRight() const { return PointF(m_x1, m_y1); }
    GX_NODISCARD PointF center() const;
    //! Get enough integer Rect object to surround this RectF.
    GX_NODISCARD Rect bounded() const;

    void setX(float x) { m_x1 += x - m_x0, m_x0 = x; }
    void setY(float y) { m_y1 += y - m_y0, m_y0 = y; }
    void setWidth(float width);
    void setHeight(float height);
    void setTop(float top) { m_y0 = top; }
    void setLeft(float left) { m_x0 = left; }
    void setBottom(float bottom) { m_y1 = bottom; }
    void setRight(float right) { m_x1 = right; }
    void reset();
    operator Rect() const;

private:
    float m_x0, m_y0, m_x1, m_y1;
};

class Line {
public:
    Line() GX_DEFAULT;
    Line(int x1, int y1, int x2, int y2);
    Line(const Point &point1, const Point &point2);
    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD int x1() const { return m_p1.x(); }
    GX_NODISCARD int y1() const { return m_p1.y(); }
    GX_NODISCARD int x2() const { return m_p2.x(); }
    GX_NODISCARD int y2() const { return m_p2.y(); }
    GX_NODISCARD const Point &point1() const { return m_p1; }
    GX_NODISCARD const Point &point2() const { return m_p2; }
    GX_NODISCARD int dx() const { return m_p2.x() - m_p1.x(); }
    GX_NODISCARD int dy() const { return m_p2.y() - m_p1.y(); }
    void setPoint1(const Point &point) { m_p1 = point; }
    void setPoint2(const Point &point) { m_p2 = point; }
    void translate(const Point &offset);
    void translate(int dx, int dy);
    void reset();
    bool operator==(const Line &other) const;
    bool operator!=(const Line &other) const;
    Line operator+(const Point &offset) const;

private:
    Point m_p1, m_p2;
};

class LineF {
public:
    enum IntersectType { NoIntersection, BoundedIntersection, UnboundedIntersection };
    LineF() GX_DEFAULT;
    LineF(float x1, float y1, float x2, float y2);
    LineF(const PointF &point1, const PointF &point2);
    LineF(const Line &line);
    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD bool fuzzyEmpty() const;
    GX_NODISCARD float x1() const { return m_p1.x(); }
    GX_NODISCARD float y1() const { return m_p1.y(); }
    GX_NODISCARD float x2() const { return m_p2.x(); }
    GX_NODISCARD float y2() const { return m_p2.y(); }
    GX_NODISCARD const PointF &point1() const { return m_p1; }
    GX_NODISCARD const PointF &point2() const { return m_p2; }
    GX_NODISCARD float dx() const { return m_p2.x() - m_p1.x(); }
    GX_NODISCARD float dy() const { return m_p2.y() - m_p1.y(); }
    void setPoint1(const PointF &point) { m_p1 = point; }
    void setPoint2(const PointF &point) { m_p2 = point; }
    void translate(const PointF &offset);
    void translate(float dx, float dy);
    void reset();
    void setLength(float len);
    GX_NODISCARD LineF unitVector() const;
    GX_NODISCARD LineF normalVector() const;
    GX_NODISCARD float angle() const;
    GX_NODISCARD float angleTo(const LineF &l) const;
    GX_NODISCARD float length() const;
    GX_NODISCARD float length2() const;
    GX_NODISCARD IntersectType intersects(const LineF &l, PointF *point) const;
    bool operator==(const LineF &other) const;
    bool operator!=(const LineF &other) const;
    operator Line() const;

private:
    PointF m_p1, m_p2;
};

class Margin {
public:
    Margin() : m_left(), m_top(), m_right(), m_bottom() {}
    Margin(int margin);
    Margin(int horizontal, int vertical);
    Margin(int left, int top, int right, int bottom);

    GX_NODISCARD bool isNull() const;
    GX_NODISCARD int left() const { return m_left; }
    GX_NODISCARD int top() const { return m_top; }
    GX_NODISCARD int right() const { return m_right; }
    GX_NODISCARD int bottom() const { return m_bottom; }
    void setLeft(int left) { m_left = int16_t(left); }
    void setTop(int top) { m_top = int16_t(top); }
    void setRight(int right) { m_right = int16_t(right); }
    void setBottom(int bottom) { m_bottom = int16_t(bottom); }
    void reset() { m_left = m_right = m_top = m_bottom = 0; }

    bool operator==(const Margin &rhs) const;
    bool operator!=(const Margin &rhs) const { return !(*this == rhs); }
    Margin operator+(const Margin &margin) const;
    Margin &operator+=(const Margin &margin);

private:
    int16_t m_left, m_top, m_right, m_bottom;
};

class Length {
public:
    enum Unit { Auto, LogicPixel, Pixel, Percent, Point, Rem, UnknownUnit };
    enum AutoAlign { AutoStart, AutoEnd };
    typedef int FP26Q6;
    static const int Maximum = 2097151;

    Length() : m_value(), m_unit() {}
    explicit Length(const char *value);
    Length(int value);
    Length(float value, Unit unit = Pixel);
    Length(FP26Q6 value, Unit unit) : m_value(value), m_unit(unit) {}
    GX_NODISCARD FP26Q6 value() const;
    GX_NODISCARD float valueF() const { return float(value()) / 64; }
    GX_NODISCARD Unit unit() const { return Unit(m_unit); }
    GX_NODISCARD bool isAuto() const { return unit() == Auto; }
    GX_NODISCARD bool isPercent() const { return unit() == Percent; }
    GX_NODISCARD int resolve(int parentValue, AutoAlign autoAlign = AutoStart) const;
    GX_NODISCARD float resolve(float parentValue, AutoAlign autoAlign = AutoStart) const;
    GX_NODISCARD const char *unitName() const { return unitName(unit()); }
    GX_NODISCARD Length blend(const Length &x, float progress) const;
    Length operator*(float scale) const;
    Length operator+(const Length &rhs) const;
    Length operator+(int rhs) const;
    Length operator+(float rhs) const;
    bool operator==(const Length &rhs) const;
    bool operator!=(const Length &rhs) const;
    bool operator==(int rhs) const;
    bool operator!=(int rhs) const;
    bool operator==(float rhs) const;
    bool operator!=(float rhs) const;

    static const char *unitName(Unit unit);
    static Length fromPercent(float value) { return Length(value, Percent); }
    static Length fromPoint(float value) { return Length(value, Point); }
    static Length fromRem(float value) { return Length(value, Rem); }
    static Length fromAuto() { return Length(0, Auto); }

private:
    uint32_t m_value : 28; // Q26.6 fixed point
    uint32_t m_unit : 4;
};

Rect operator*(const Rect &lhs, float rhs);
Rect operator+(const Rect &lhs, const Rect &rhs);
Rect &operator+=(Rect &rect, const Margin &margin);
Rect &operator-=(Rect &rect, const Margin &margin);
RectF operator*(const RectF &lhs, float rhs);
RectF operator+(const RectF &lhs, const RectF &rhs);
RectF &operator+=(RectF &rect, const Margin &margin);
Size &operator+=(Size &size, const Margin &margin);
RectF &operator-=(RectF &rect, const Margin &margin);
Size &operator-=(Size &size, const Margin &margin);

inline Size Size::operator&(const Size &other) const { return intersected(other); }

inline Size Size::operator|(const Size &other) const { return united(other); }

inline Point operator*(const Point &lhs, float rhs) {
    return Point(int(float(lhs.x()) * rhs), int(float(lhs.y()) * rhs));
}

inline Point operator*(const Point &lhs, int rhs) { return Point(lhs.x() * rhs, lhs.y() * rhs); }

inline bool operator==(const PointF &lhs, const PointF &rhs) {
    return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}

inline bool operator!=(const PointF &lhs, const PointF &rhs) { return !(lhs == rhs); }

inline PointF operator+(const PointF &lhs, const PointF &rhs) {
    return PointF(lhs.x() + rhs.x(), lhs.y() + rhs.y());
}

inline PointF operator-(const PointF &lhs, const PointF &rhs) {
    return PointF(lhs.x() - rhs.x(), lhs.y() - rhs.y());
}

inline PointF operator*(const PointF &lhs, const PointF &rhs) {
    return PointF(lhs.x() * rhs.x(), lhs.y() * rhs.y());
}

inline PointF operator/(const PointF &lhs, const PointF &rhs) {
    return PointF(lhs.x() / rhs.x(), lhs.y() / rhs.y());
}

inline PointF operator*(const PointF &lhs, int rhs) {
    return PointF(lhs.x() * float(rhs), lhs.y() * float(rhs));
}

inline PointF operator*(const PointF &lhs, float rhs) {
    return PointF(lhs.x() * rhs, lhs.y() * rhs);
}

inline PointF operator/(const PointF &lhs, float rhs) {
    return PointF(lhs.x() / rhs, lhs.y() / rhs);
}

inline PointF operator-(const PointF &p) { return PointF(-p.x(), -p.y()); }

inline float PointF::dotProduct(const PointF &b) const { return x() * b.x() + y() * b.y(); }

inline float PointF::crossProduct(const PointF &b) const { return x() * b.y() - b.x() * y(); }

inline bool PointF::isAcute(const PointF &b) const {
    const float epsilon = -4e-6f;
    return dotProduct(b) >= epsilon && crossProduct(b) >= epsilon;
}

inline Rect Rect::operator&(const Rect &other) const { return intersected(other); }

inline Rect Rect::operator|(const Rect &other) const { return united(other); }

inline LineF::LineF(const Line &line) : m_p1(line.point1()), m_p2(line.point2()) {}

inline float LineF::length2() const {
    const float x = dx(), y = dy();
    return x * x + y * y;
}

inline bool LineF::isEmpty() const { return dx() == 0 && dy() == 0; }

inline bool LineF::fuzzyEmpty() const { return fuzzyZero(dx()) && fuzzyZero(dy()); }

inline void LineF::setLength(float len) {
    if (fuzzyEmpty())
        return;
    LineF v = unitVector();
    m_p2.setPoint(m_p1.x() + v.dx() * len, m_p1.y() + v.dy() * len);
}

inline LineF LineF::normalVector() const { return LineF(m_p1, m_p1 + PointF(dy(), -dx())); }

inline LineF::operator Line() const { return Line(point1(), point2()); }

inline Margin Margin::operator+(const Margin &margin) const {
    Margin result(*this);
    return result += margin;
}

inline Rect operator+(const Rect &rect, const Margin &margin) {
    Rect result(rect);
    return result += margin;
}

inline Rect operator-(const Rect &rect, const Margin &margin) {
    Rect result(rect);
    return result -= margin;
}

inline RectF operator+(const RectF &rect, const Margin &margin) {
    RectF result(rect);
    return result += margin;
}

inline RectF operator-(const RectF &rect, const Margin &margin) {
    RectF result(rect);
    return result -= margin;
}

inline Size operator+(const Size &size, const Margin &margin) {
    Size result(size);
    return result += margin;
}

inline Size operator-(const Size &size, const Margin &margin) {
    Size result(size);
    return result -= margin;
}

inline Length::FP26Q6 Length::value() const {
    return FP26Q6(m_value & (1 << 27) ? m_value | 0xf << 28 : m_value);
}

inline bool Length::operator==(const Length &rhs) const {
    return m_unit == rhs.m_unit && m_value == rhs.m_value;
}

inline bool Length::operator!=(const Length &rhs) const { return !(*this == rhs); }

inline bool Length::operator==(int rhs) const { return m_unit == Pixel && m_value == rhs * 64; }

inline bool Length::operator!=(int rhs) const { return !(*this == rhs); }

inline bool Length::operator==(float rhs) const {
    return m_unit == Pixel && m_value == int(rhs * 64);
}

inline bool Length::operator!=(float rhs) const { return !(*this == rhs); }

const Logger &operator<<(const Logger &, const Size &);
const Logger &operator<<(const Logger &, const Point &);
const Logger &operator<<(const Logger &, const PointF &);
const Logger &operator<<(const Logger &, const Rect &);
const Logger &operator<<(const Logger &, const RectF &);
const Logger &operator<<(const Logger &, const Line &);
const Logger &operator<<(const Logger &, const LineF &);
const Logger &operator<<(const Logger &, const Margin &);
const Logger &operator<<(const Logger &, const Length &);
} // namespace gx
