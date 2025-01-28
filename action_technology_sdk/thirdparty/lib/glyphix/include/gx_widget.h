/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_font.h"
#include "gx_geometry.h"
#include "gx_object.h"
#include "gx_style.h"
#include "gx_styles.h"

GX_INCLUDE("gx_transformoperations.h")

namespace gx {
class Layout;
class Painter;
class MoveEvent;
class ResizeEvent;
class PaintEvent;
class LayoutEvent;
class GestureEvent;
class WheelEvent;
class KeyEvent;
class FocusEvent;
class StyleModifier;

//! The base class for all widgets.
class Widget : public Object {
    GX_OBJECT

public:
    enum Flags {
        SizeHintCached = UserObjectFlags << 0,
        HeightForWidthCached = UserObjectFlags << 1,
        PendingLayout = UserObjectFlags << 2,
        DisableGesture = UserObjectFlags << 3,
        SnapshotSelf = UserObjectFlags << 4,
        SnapshotChild = UserObjectFlags << 5,
        UserWidgetFlags = UserObjectFlags << 6
    };
    enum FocusPolicy {
        NoFocus = 0,
        ClickFocus = 1,
    };
    GX_FLAGS(FocusPolicy);
    enum RenderFlags { NoRenderFlags = 0, DrawChild = 1 << 0, UpdateShowing = 1 << 1 };
    GX_FLAGS(RenderFlags);
    enum PositionPolicy { StaticPosition = 0, AbsolutePosition = 1 };
    /**
     * The layout hints by Widget objects.
     * @see layoutHints()
     */
    enum LayoutHints {
        //! Resolve size only by sizeHint() without the \p width and \p height of the style.
        IgnoreStyleSize = 1 << 0,
        //! The user defined layout hints.
        UserLayoutHints = 1 << 8
    };

    explicit Widget(Widget *parent = nullptr);
    virtual ~Widget();

    GX_NODISCARD int minimumWidth() const;
    GX_NODISCARD int minimumHeight() const;
    GX_NODISCARD int maximumWidth() const;
    GX_NODISCARD int maximumHeight() const;
    GX_NODISCARD Size minimumSize() const;
    GX_NODISCARD Size maximumSize() const;
    GX_NODISCARD const Margin &padding() const { return d.padding; }
    GX_NODISCARD const Margin &border() const { return d.border; }
    GX_NODISCARD const Margin &margin() const { return d.margin; };
    GX_NODISCARD const Font &font() const;
    GX_NODISCARD float opacity() const { return style().opacity(); }
    GX_NODISCARD int x() const { return geometry().x(); }
    GX_NODISCARD int y() const { return geometry().y(); }
    GX_NODISCARD int zIndex() const { return d.zIndex; }
    GX_NODISCARD int width() const { return geometry().width(); }
    GX_NODISCARD int height() const { return geometry().height(); }
    GX_NODISCARD Point position() const { return geometry().topLeft(); }
    GX_NODISCARD Size size() const { return geometry().size(); }
    GX_NODISCARD Rect rect() const { return Rect(0, 0, geometry().width(), geometry().height()); }
    GX_NODISCARD const Rect &geometry() const { return d.rect; }
    GX_NODISCARD Rect boundedRect() const { return rect() + d.margin + d.border + d.padding; }
    GX_NODISCARD Rect boundedGeometry() const {
        return geometry() + d.margin + d.border + d.padding;
    }
    GX_NODISCARD Margin boundedMargin() const { return d.margin + d.border + d.padding; }
    GX_NODISCARD Rect mapToGlobal(const Rect &rect) const;
    GX_NODISCARD Point mapToGlobal(const Point &point) const;
    GX_NODISCARD Rect mapToParent(const Rect &rect) const { return mapToParent(rect, parent()); }
    GX_NODISCARD Point mapToParent(const Point &point) const {
        return mapToParent(point, parent());
    }
    GX_NODISCARD Rect mapToParent(const Rect &rect, const Widget *widget) const;
    GX_NODISCARD Point mapToParent(const Point &point, const Widget *widget) const;
    GX_NODISCARD bool isLazySnapshot() const { return d.lazySnapshot; }
    GX_NODISCARD bool isInlineWidget() const { return d.inlineWidget; }
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int width) const;
    virtual Line inlineSpan(int width, int indent);
    int baseline() const;
    /**
     * The layout hints from this widget.
     * @return The bits are from by \ref LayoutHints.
     * @see LayoutHints
     */
    GX_NODISCARD virtual int layoutHints() const;

    void setX(int x) { setPosition(Point(x, y())); }
    void setY(int y) { setPosition(Point(x(), y)); }
    void setZIndex(int index);
    void setWidth(int width) { setSize(Size(width, height())); }
    void setHeight(int height) { setSize(Size(width(), height)); }
    void setPosition(int x, int y) { setPosition(Point(x, y)); }
    void setPosition(const Point &position) { setGeometry(Rect(position, size())); }
    void setSize(int width, int height) { setSize(Size(width, height)); }
    void setSize(const Size &size) { setGeometry(Rect(position(), size)); }
    void setRect(int x, int y, int width, int height) { setRect(Rect(x, y, width, height)); }
    void setRect(const Rect &rect) { setSize(rect.size()); }
    void setGeometry(int x, int y, int width, int height);
    void setGeometry(const Rect &rect);

    /**
     * Set the padding of a widget. This function changes the style of the widge, not just its
     * geometry.
     */
    void setPadding(const Margin &padding);
    /**
     * Set the margin of a widget. This function changes the style of the widge, not just its
     * geometry.
     */
    void setMargin(const Margin &margin);

    GX_NODISCARD Widget *parent() const;
    void setParent(Widget *parent);
    void setParent(Widget *parent, int position);

    GX_NODISCARD bool isVisible() const { return d.visible; }
    /**
     * Gets whether the widget is transparent or not. The transparent widget participate in the
     * layout but are not drawn.
     * @see setTransparent()
     * @see isVisible()
     */
    GX_NODISCARD bool isTransparent() const { return d.transparent; }
    /**
     * Gets whether the widget has been disabled.
     * @see setDisabled()
     */
    GX_NODISCARD bool isDisabled() const { return d.state == Styles::Disabled; }
    /**
     * In contrast to isVisible(), isShowing() checks if widget is being displayed on the screen,
     * i.e. the widget must be a child of a window and all its parents are visible and painted.
     * The above conditions will be retested if the widget is removed or rejoined from the window's
     * widget tree, or any parent is not invisible.
     */
    GX_NODISCARD bool isShowing() const;
    GX_NODISCARD bool isFocus() const;
    GX_NODISCARD FocusPolicy focusPolicy() const { return FocusPolicy(d.focusPolicy); }
    GX_NODISCARD bool isTransformed() const { return d.hasTransform; }
    GX_NODISCARD bool isPercentMargins() const { return d.percentMargins; }
    /**
     * Returns whether the widget defaults to a flow-layout. If the layout() is null, the default
     * layout (flow-layout or no-layout) is used.
     * @see setFlowLayout()
     */
    GX_NODISCARD bool isFlowLayout() const { return d.flowLayout && !children().empty(); }
    GX_NODISCARD PositionPolicy positionPolicy() const;
    GX_NODISCARD virtual Layout *layout() const;

    /**
     * Sets the visibility state of the widget. The visibility state determines whether the widget
     * is visible on the screen or not.
     * @param state The new visibility state of the widget. True for visible, false for invisible.
     * @note A layout update is requested if the widget becomes visible.
     */
    virtual void setVisible(bool state);
    /**
     * Sets the layout for the widget. If the layout is different than the current layout, the
     * previous layout is deleted and the new layout is assigned to the widget.
     * @param layout The layout to be set for the widget.
     */
    virtual void setLayout(Layout *layout);
    /**
     * Sets the position policy for the widget. The position policy determines how the widget's
     * position is handled within its parent layout.
     * @param policy The position policy to be set for the widget.
     * @remark This function updates the parent layout of the widget and marks the widget as dirty.
     */
    void setPositionPolicy(PositionPolicy policy);
    /**
     * Set the lazy snapshot option, the update() call will not trigger a snapshot update when the
     * lazy snapshot is enabled. This may cause the snapshot content to lag, but increase the FPS.
     */
    void setLazySnapshot(bool enable) { d.lazySnapshot = enable; }
    void setFocusPolicy(FocusPolicy policy);
    void setFocus(bool enable = true);
    /**
     * Set the widget to an inline element or a block element. This property is only used in flow
     * Layout.
     * @param enable The widget is inline elements when true, otherwise block element.
     */
    void setInlineWidget(bool enable);
    /**
     * Set whether the widget is the default to flow-layout.
     * @see isFlowLayout()
     */
    void setFlowLayout(bool enable);
    /**
     * Set whether to disable the widget. Disabled widgets do not respond to gestures.
     * @see isDisabled()
     */
    void setDisabled(bool disabled);
    /**
     * Sets whether the widget is transparent or not. The transparent widget participate in the
     * layout but are not drawn.
     * @see isTransparent()
     * @see setVisible()
     */
    void setTransparent(bool transparent);

    GX_NODISCARD const Style &style(Styles::PartType part = Styles::Content) const;
    GX_NODISCARD const Style &inlineStyle() const;
    GX_NODISCARD const Styles &styles() const;
    void setStyle(const Style &style);
    void setStyles(const Styles &styles);
    /**
     * Set the font of the widget. This setting may be overridden by the style() and will also be
     * cleared when Application::setFont() is called.
     * @see Application::setFont()
     */
    void setFont(const Font &font);
    void setOpacity(float opacity);
    GX_NODISCARD StyleAnimation *styleAnimation(Styles::PartType part = Styles::Content);

    void addItem(Widget *widget) { addItem(widget, -1); }
    virtual void addItem(Widget *widget, int index);

    //! Check if the current widget (tree) has been invalidated.
    bool isInvalidated() const;
    /**
     * Mark the widget as invalidate, which causes the widget to redraw. This function needs to be
     * called when the widget's content is updated. This function does not trigger layout update,
     * and updateLayout() should be called when a layout update is required.
     * @see updateLayout()
     */
    void update();
    /**
     * Updates the layout of the widget and its parent widgets. The actual layout is deferred until
     * the event loop. If the layout is currently being executed, the function returns early and
     * does not perform any update.
     */
    void updateLayout();

    virtual bool event(Event *event);

    /**
     * Renders the widget onto the provided Painter. Use this function to draw widget to any
     * surfaces, such as Pixmap.
     * @param painter The painter object used for rendering.
     * @param offset The offset to be applied to the widget's position.
     * @param clip The clipping rectangle applied to the painter.
     * @param flags Flag indicating whether to render widgets.
     */
    void render(Painter *painter, const Point &offset = Point(), const Rect &clip = Rect(),
                RenderFlags flags = DrawChild);

    GX_PROPERTY(Length top, set setStyleTop, get styleTop)
    GX_PROPERTY(Length left, set setStyleLeft, get styleLeft)
    GX_PROPERTY(Length width, set setStyleWidth, get styleWidth)
    GX_PROPERTY(Length height, set setStyleHeight, get styleHeight)
    GX_PROPERTY(int zIndex, set setZIndex, get zIndex)
    GX_PROPERTY(TransformOperations transform, set setStyleTransform, get styleTransform)
    GX_PROPERTY(Rect rect, set setRect, get rect)
    GX_PROPERTY(Rect geometry, set setGeometry, get geometry)
    GX_PROPERTY(Rect boundedRect, get boundedRect)
    GX_PROPERTY(bool show, set setVisible, get isVisible)
    GX_PROPERTY(bool disabled, set setDisabled, get isDisabled)
    GX_PROPERTY(float opacity, set setOpacity, get opacity)
    GX_PROPERTY(Style style, set setStyle, get inlineStyle)
    GX_PROPERTY(bool lazySnapshot, set setLazySnapshot, get isLazySnapshot)
    GX_PROPERTY(bool focus, set setFocus, get isFocus)

protected:
    GX_NODISCARD Styles::StateType state() const { return Styles::StateType(d.state); }
    void setState(Styles::StateType state);
    void requestNextTick();
    void setSnapshotBypass(bool enable);

    virtual void updateStyle();
    virtual int baseline(int) const;
    virtual void paintEvent(PaintEvent *event);
    virtual bool gestureEvent(GestureEvent *event);
    virtual bool wheelEvent(WheelEvent *event);
    virtual bool keyEvent(KeyEvent *event);
    virtual bool layoutEvent(LayoutEvent *event);
    virtual void resizeEvent(ResizeEvent *event);
    virtual void moveEvent(MoveEvent *event);
    virtual bool focusEvent(FocusEvent *event);

private:
    void updateStyleP();
    void setupStyle();
    void addDirty();
    void markChain(int flags);
    GX_NODISCARD Length styleTop() const { return y() - boundedMargin().top(); }
    GX_NODISCARD Length styleLeft() const { return x() - boundedMargin().left(); }
    GX_NODISCARD Length styleWidth() const { return width(); }
    GX_NODISCARD Length styleHeight() const { return height(); }
    GX_NODISCARD TransformOperations styleTransform() const;
    void setStyleTop(const Length &top);
    void setStyleLeft(const Length &left);
    void setStyleWidth(const Length &width);
    void setStyleHeight(const Length &height);
    void setStyleTransform(const TransformOperations &transform);
    void setZIndexSimple(int zIndex);
    void styleAnimationFinished();

    struct Private {
        // geometry & layout
        Layout *layout;
        Rect rect;
        Size sizeHint;       // layout cached sizeHint
        Size heightForWidth; // layout cached heightForWidth
        Margin margin, border, padding;
        // style
        Styles styles;
        StyleSet activeStyle;
        Font font;
        mutable uint16_t baseline;
        int8_t zIndex;
        uint8_t needsZIndexSorting : 1;
        uint8_t state : 6; // style state
        // status & flags
        uint32_t visible : 1;
        uint32_t visibleChanged : 1; // The visible status has changed.
        uint32_t positionPolicy : 1;
        uint32_t layoutDirection : 2;
        uint32_t unstyled : 1;
        uint32_t focusPolicy : 2;
        uint32_t hasOpacity : 1;
        uint32_t hasTransform : 1;
        uint32_t hasFilter : 1;
        uint32_t transparent : 1;
        uint32_t percentMargins : 1;
        uint32_t snapshotCached : 1;
        uint32_t lazySnapshot : 1;
        uint32_t dirty : 1;
        uint32_t inlineWidget : 1;
        uint32_t flowLayout : 1;
        uint32_t marks : 8; // private marks

        Private();
        ~Private();
    };
    Private d;
    friend class Object;
    friend class Layout;
    friend class WidgetManager;
    friend class WidgetZOrder;
    friend class GestureManager;
    friend class SnapshotCache;
    friend class StyleAnimation;
    friend class Application;
    friend class StyleModifier;
};

class StyleModifier {
public:
    explicit StyleModifier(Widget *widget);
    ~StyleModifier();
    Style *operator->() { return m_style; }

private:
    Widget *m_widget;
    Style *m_style;
};

// NOLINTNEXTLINE(readability-*)
template<> GX_NODISCARD inline Widget *dyn_cast<Widget *>(Object *object) {
    return object && object->isWidget() ? static_cast<Widget *>(object) : nullptr;
}

// NOLINTNEXTLINE(readability-*)
template<> GX_NODISCARD inline const Widget *dyn_cast<const Widget *>(const Object *object) {
    return object && object->isWidget() ? static_cast<const Widget *>(object) : nullptr;
}

inline Widget::PositionPolicy Widget::positionPolicy() const {
    return static_cast<PositionPolicy>(d.positionPolicy);
}

inline Widget *Widget::parent() const { return dyn_cast<Widget *>(Object::parent()); }

inline void Widget::setParent(Widget *parent) { Object::setParent(parent); }

inline void Widget::setParent(Widget *parent, int position) { Object::setParent(parent, position); }

inline void Widget::setGeometry(int x, int y, int width, int height) {
    setGeometry(Rect(x, y, width, height));
}

const Logger &operator<<(const Logger &, const Widget *);
} // namespace gx
