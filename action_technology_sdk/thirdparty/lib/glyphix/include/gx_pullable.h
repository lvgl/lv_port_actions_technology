#pragma once

#include "gx_pointer.h"
#include "gx_signals.h"
#include "gx_widget.h"

namespace gx {
class AbstractScrollArea;

class Pullable : public Widget {
    GX_OBJECT

public:
    explicit Pullable(Widget *parent = nullptr);
    virtual ~Pullable();

    bool isPulling() const { return false; }
    bool isTriggered() const { return false; }
    bool isHolding() const { return m_holding; }
    void setHolding(bool enable);
    virtual bool event(Event *event);

    Signal<> pulling;

    GX_PROPERTY(bool hold, get isHolding, set setHolding);
    GX_PROPERTY(bool release, get isPulling, signal pulling);

private:
    void onScrolled(const Point &offset, bool scrolling);
    void onContentResized();

private:
    Pointer<AbstractScrollArea> m_scrollArea;
    int m_oldBottom;
    bool m_visible;
    bool m_holding;
    bool m_pulling;
};
} // namespace gx
