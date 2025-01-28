/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_color.h"
#include "gx_geometry.h"
#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_variant.h"
#include "gx_vector.h"

namespace gx {
class Font;
class Brush;
class Color;
class Layout;
class Variant;
class Object;
class Widget;
class TransformOperations;
class EaseCurveFactory;
class StyleKeyframes;
class StyleAnimation;
class FontFaceMap;

class Style {
public:
    // NOTE: The enum Role names must be in lexicographical order!
    enum Role {
        SR_AlignContent,
        SR_AlignItems,
        SR_AlignSelf,
        SR_Animation,
        SR_BackdropFilter,
        SR_Background,
        SR_BackgroundColor,
        SR_Border,
        SR_BorderRadius,
        SR_Bottom,
        SR_Color,
        SR_Display,
        SR_Filter,
        SR_Flex,
        SR_FlexDirection,
        SR_FlexFlow,
        SR_FlexGrow,
        SR_FlexShrink,
        SR_FlexWrap,
        SR_FontFamily,
        SR_FontSize,
        SR_Height,
        SR_JustifyContent,
        SR_Left,
        SR_LineHeight,
        SR_Margin,
        SR_MaxHeight,
        SR_MaxLines,
        SR_MaxWidth,
        SR_MinHeight,
        SR_MinWidth,
        SR_ObjectFit,
        SR_Opacity,
        SR_Padding,
        SR_Position,
        SR_Right,
        SR_ScrollSnapAlign,
        SR_ScrollSnapType,
        SR_StrokeWidth,
        SR_TextAlign,
        SR_TextOverflow,
        SR_Top,
        SR_Transform,
        SR_Transparent,
        SR_Visibility,
        SR_Width,
        SR_ZIndex,
        SR_UnknownRole
    };
    enum ScrollSnapAlign { SSA_None, SSA_Start, SSA_End, SSA_Center };
    enum ScrollSnapType { SST_None, SST_XMandatory, SST_YMandatory };
    enum BorderFlags {
        BorderTop = 0,
        BorderRight = 1,
        BorderBottom = 2,
        BorderLeft = 3,
        BorderAll = 4
    };

    class const_iterator;
    class Margin;
    class FlexLength;
    class FlexFlow;
    class Border;

    Style() GX_DEFAULT;
    explicit Style(const Style *base);
    Style(const Style &other);
#ifndef GX_COPMATIBLE_CXX_98
    Style(Style &&other) noexcept;
#endif
    ~Style();

    /**
     * Check if a style object is initial value.
     * @return Returns true if the style has no base style and no own property.
     */
    GX_NODISCARD bool isNull() const { return d.empty(); }
    /**
     * Check that the style does not contain any properties.
     * @return Returns true if the style has no own property and the base style is also empty.
     */
    GX_NODISCARD bool isEmpty() const;
    /**
     * Get the base style. The style object will inherit property from the base style.
     * @return The base style object for the style.
     * @see setBase()
     */
    GX_NODISCARD const Style &base() const;
    /**
     * Sets the base style of the style.
     * @param style The base style.
     * @note Setting the base style to itself (e.g. `style.setBase(style)`) will reset the base
     * style, which is equivalent to `style.setBase(Style())`.
     * @see base()
     */
    void setBase(const Style &style);

    /**
     * Check if the specified property exists in the style.
     * @param role The role enumeration value for the style attribute.
     * @return Returns true if the property is owned in the style or the base style.
     */
    GX_NODISCARD bool hasProperty(Role role) const;
    /**
     * Check if the specified attribute exists in the style itself.
     * @param role The role enumeration value for the style attribute.
     * @return Returns true if the property is owned in the style.
     */
    GX_NODISCARD bool hasOwnProperty(Role role) const;
    GX_NODISCARD Variant property(Role role) const;
    GX_NODISCARD Variant property(Role role, const Widget *owner) const;
#ifndef GX_COPMATIBLE_CXX_98
    void setProperty(Role role, Variant &&value);
    GX_DEPRECATED_MSG("use setProperty(Role, Variant&&) instead")
    void setProperty(Role role, const Variant &value) { setProperty(role, Variant(value)); }
#else
    void setProperty(Role role, const Variant &value);
#endif
    /**
     * Deletes the specified style property, but not the base style attribute.
     * @param role The role of property to be deleted.
     * @return Whether the deletion was successful.
     */
    bool deleteProperty(Role role);
    /**
     * Clearing all own attributes except base-style. This function also does not stop the style
     * animation.
     */
    void clearOwnProperty();
    /**
     * Reset this style object, equivalent to `*this = Style()`.
     */
    void reset();

    GX_NODISCARD const_iterator begin() const;
    GX_NODISCARD const_iterator end() const;

    static const Style &fromEmpty();

    // geometry properties
    GX_NODISCARD Length width() const;
    GX_NODISCARD Length height() const;
    GX_NODISCARD Length minWidth() const;
    GX_NODISCARD Length minHeight() const;
    GX_NODISCARD Length maxWidth() const;
    GX_NODISCARD Length maxHeight() const;
    GX_NODISCARD Margin padding() const;
    GX_NODISCARD Margin margin() const;
    GX_NODISCARD Border border() const;
    GX_NODISCARD Length borderRadius() const;
    GX_NODISCARD Length lineHeight() const;
    GX_NODISCARD Length strokeWidth() const;
    GX_NODISCARD int textFlags(const Widget *owner = nullptr) const;
    GX_NODISCARD int alignItems() const;
    GX_NODISCARD int alignSelf() const;
    GX_NODISCARD int justifyContent() const;
    GX_NODISCARD float opacity() const;
    GX_NODISCARD TransformOperations transform() const;
    GX_NODISCARD bool transparent() const;
    GX_NODISCARD int imageFlags() const;
    GX_NODISCARD int display() const;
    GX_NODISCARD bool visibility() const;
    GX_NODISCARD std::pair<String, FontFaceMap> fontFamily() const;
    GX_NODISCARD Length fontSize() const;
    GX_NODISCARD FlexLength flex() const;
    GX_NODISCARD FlexFlow flexFlow() const;
    GX_NODISCARD int maxLines() const;
    GX_NODISCARD ScrollSnapAlign scrollSnapAlign() const;
    GX_NODISCARD ScrollSnapType scrollSnapType() const;

    void setWidth(const Length &value);
    void setHeight(const Length &value);
    void setSize(const Length &width, const Length &height);
    void setMinWidth(const Length &value);
    void setMinHeight(const Length &value);
    void setMinSize(const Length &width, const Length &height);
    void setMaxWidth(const Length &value);
    void setMaxHeight(const Length &value);
    void setMaxSize(const Length &width, const Length &height);
    void setPadding(const Margin &margin);
    void setPadding(int left, int top, int right, int bottom);
    void setBorder(const Border &pen);
    void setBorder(const Length &size, const Color &color, BorderFlags flags = BorderAll);
    void setMargin(const Margin &margin);
    void setMargin(int left, int top, int right, int bottom);
    void setBorderRadius(const Length &radius);
    void setLineHeight(const Length &value);
    void setTextFlags(int flags);
    void setAlignItems(int mode);
    void setAlignSelf(int mode);
    void setJustifyContent(int mode);
    void setOpacity(float value);
    void setTransform(const TransformOperations &transform);
    void setTransparent(bool status);
    void setImageFlags(int flags);
    void setDisplay(int type);
    void setVisibility(bool type);
    void setFontFamily(const String &family);
    void setFontFamily(const String &family, const FontFaceMap &faces);
    void setFontSize(const Length &size);

    // paint properties
    GX_NODISCARD Brush background() const;
    GX_NODISCARD Color color() const;
    void setBackground(const Brush &brush);
    void setColor(const Color &color);

    // animation & layout
    void setAnimation(const StyleKeyframes &keyframes);
    void setFlex(const FlexLength &flex);
    void setFlex(int grow = 0, int shrink = 1, bool basis = true);
    void setFlexFlow(const FlexFlow &flow);
    GX_NODISCARD Layout *newLayout() const;
    GX_NODISCARD StyleAnimation *animation() const;
    /**
     * Creates and starts a style animation, which is defined by the `animation` or `transition`
     * properties.
     * @return The style animation object, or nullptr if there are no animation properties.
     * @see createAnimation()
     */
    StyleAnimation *startAnimation();
    /**
     * Creates a style animation for this style object that regardless of the `animation` or
     * `transition` properties. The caller needs to configure and start the animation.
     * @return The style animation object.
     * @see startAnimation()
     * @note It is better to \ref StyleAnimation::start() "start" the animation in
     * StyleAnimation::DeleteOnStop policy to free memory of the animation object as soon as
     * possible.
     */
    StyleAnimation *createAnimation();

    bool operator==(const Style &rhs) const { return d == rhs.d; }
    bool operator!=(const Style &rhs) const { return !(*this == rhs); }
    Style &operator=(const Style &other);
#ifndef GX_COPMATIBLE_CXX_98
    Style &operator=(Style &&other) noexcept;
#endif

    static std::pair<Role, Variant::type_t> role(const String &name) { return role(name.c_str()); }
    static std::pair<Role, Variant::type_t> role(const char *name);
    static std::pair<const char *, Variant::type_t> role(Style::Role role);

private:
    bool detach();
    bool growing();
    void moveAnimation();
    void stopAnimation();

private:
    class Data;
    GX_SHARED_HELPER_DECL(Helper, Data);
    explicit Style(Data *p);
    SharedRef<Data, Helper> d;
    friend class StyleAnimation;
};

class Style::const_iterator {
public:
    typedef std::pair<Role, Vector<Variant>::const_iterator> value_type;

    const_iterator() : m_style() {}
    const_iterator(const const_iterator &other);
    const value_type &operator*() const { return m_pair; }
    const value_type *operator->() const { return &m_pair; }
    const_iterator &operator++();
    const_iterator operator++(int);
    bool operator==(const const_iterator &rhs) const { return m_pair.second == rhs.m_pair.second; }
    bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }

private:
    value_type m_pair;
    const Data *m_style;
    friend class Style;
};

inline Style::const_iterator::const_iterator(const const_iterator &other)
    : m_pair(other.m_pair), m_style(other.m_style) {}

class Style::Margin {
public:
    Margin()
        : m_left(0, Length::Pixel), m_top(0, Length::Pixel), m_right(0, Length::Pixel),
          m_bottom(0, Length::Pixel) {}
    Margin(const gx::Margin &m)
        : m_left(m.left()), m_top(m.top()), m_right(m.right()), m_bottom(m.bottom()) {}
    Margin(const Length &horizontal, const Length &vertical)
        : m_left(horizontal), m_top(vertical), m_right(horizontal), m_bottom(vertical) {}
    Margin(const Length &left, const Length &top, const Length &right, const Length &bottom)
        : m_left(left), m_top(top), m_right(right), m_bottom(bottom) {}
    GX_NODISCARD gx::Margin margin(const Size &size) const;
    GX_NODISCARD gx::Margin margin(int width, int height) const;
    GX_NODISCARD gx::Margin margin(const Widget *widget) const;
    GX_NODISCARD const Length &left() const { return m_left; }
    GX_NODISCARD const Length &top() const { return m_top; }
    GX_NODISCARD const Length &right() const { return m_right; }
    GX_NODISCARD const Length &bottom() const { return m_bottom; }
    GX_NODISCARD bool isAnyPercent() const;
    bool operator==(const Margin &rhs) const {
        return m_left == rhs.m_left && m_top == rhs.m_top && m_right == rhs.m_right &&
               m_bottom == rhs.m_bottom;
    }
    bool operator!=(const Margin &rhs) const { return !(*this == rhs); }

private:
    Length m_left, m_top, m_right, m_bottom;
};

class Style::FlexLength {
public:
    explicit FlexLength(int grow = 0, int shrink = 1, bool basis = true)
        : m_basis(basis), m_grow(grow), m_shrink(shrink) {}
    GX_NODISCARD int grow() const { return m_grow; }
    GX_NODISCARD int shrink() const { return m_shrink; }
    GX_NODISCARD bool basis() const { return m_basis; }
    void setGrow(int grow) { m_grow = uint16_t(grow); }
    void setShrink(int shrink) { m_shrink = uint16_t(shrink); }
    void setBasis(bool basis) { m_basis = basis; }

private:
    uint32_t m_basis : 1;
    uint32_t m_grow : 15;
    uint32_t m_shrink : 15;
};

class Style::FlexFlow {
public:
    explicit FlexFlow(int direction = 0, int wrap = 0) : m_direction(direction), m_wrap(wrap) {}
    GX_NODISCARD int direction() const { return m_direction; }
    GX_NODISCARD int wrap() const { return m_wrap; }
    void setDirection(int direction) { m_direction = uint8_t(direction); }
    void setWrap(int wrap) { m_wrap = uint8_t(wrap); }

private:
    uint8_t m_direction;
    uint8_t m_wrap;
};

inline const Style &Style::fromEmpty() {
    static const Style emptyStyle;
    return emptyStyle;
}

class Style::Border {
public:
    Border() : m_flags() {}
    Border(const Length &size, const Color &color, BorderFlags flags = BorderAll)
        : m_size(size), m_color(color), m_flags(flags) {}
    GX_NODISCARD bool isNull() const { return m_size.isAuto(); }
    GX_NODISCARD BorderFlags flags() const { return static_cast<BorderFlags>(m_flags); }
    GX_NODISCARD const Length &size() const { return m_size; }
    GX_NODISCARD const Color &color() const { return m_color; }
    bool operator==(const Border &other) const {
        return m_size == other.m_size && m_color == other.m_color && m_flags == other.m_flags;
    }
    bool operator!=(const Border &other) const { return !(*this == other); }

private:
    Length m_size;
    Color m_color;
    uint8_t m_flags;
};

inline Margin Style::Margin::margin(const Size &size) const {
    return margin(size.width(), size.height());
}

inline bool Style::Margin::isAnyPercent() const {
    return m_left.isPercent() || m_top.isPercent() || m_right.isPercent() || m_bottom.isPercent();
}

inline void Style::setBorder(const Length &size, const Color &color, BorderFlags flags) {
    setBorder(Border(size, color, flags));
}

inline void Style::setFlex(int grow, int shrink, bool basis) {
    setFlex(FlexLength(grow, shrink, basis));
}

const Logger &operator<<(const Logger &logger, const Style::Margin &);
} // namespace gx
