/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_color.h"
#include "gx_image.h"
#include "gx_transform.h"

namespace gx {
class Widget;
class Painter;

class StyleOption {
public:
    enum Type {
        OptionFrame = 0,
        OptionText,
        OptionTextSpan,
        OptionImage,
        OptionSwitch,
        OptionCheckBox,
        OptionRange,
        OptionArc,
        OptionPicker,
        OptionScrollBar
    };
    enum ElementCategory {
        CategoryGeneral = 0,
        CategoryLabel,
        CategoryImage,
        CategoryButton,
        CategoryProgress
    };

    GX_NODISCARD Type option() const { return m_option; }

protected:
    explicit StyleOption(Type option, int category = CategoryGeneral)
        : m_option(option), m_category(category) {}
    GX_NODISCARD int elementCategory() const { return m_category; }

private:
    const Type m_option;
    const int m_category;
};

struct StyleOptionFrame : StyleOption {
    explicit StyleOptionFrame(int category = CategoryGeneral)
        : StyleOption(OptionFrame, category) {}
};

struct StyleOptionText : StyleOption {
    explicit StyleOptionText(int category = CategoryGeneral) : StyleOption(OptionText, category) {}
    String text;
};

struct StyleOptionTextSpan : StyleOption {
    explicit StyleOptionTextSpan(int category = CategoryGeneral)
        : StyleOption(OptionTextSpan, category), indent() {}
    String text;
    int indent;
};

struct StyleOptionSwitch : StyleOption {
    explicit StyleOptionSwitch(int category = CategoryGeneral)
        : StyleOption(OptionSwitch, category), scale(), transition() {}
    float scale;
    float transition;
};

struct StyleOptionCheckBox : StyleOption {
    enum Shape { CheckBox, RadioButton };
    explicit StyleOptionCheckBox(int category = CategoryGeneral)
        : StyleOption(OptionCheckBox, category), shape(), scale(1), checked() {}
    Shape shape;
    float scale;
    float checked;
};

struct StyleOptionPicker : StyleOption {
    explicit StyleOptionPicker(int category = CategoryGeneral)
        : StyleOption(OptionPicker, category), range(nullptr) {}
    Vector<String> *range;
    int position, size;
    bool loop;
};

struct StyleOptionImage : StyleOption {
    explicit StyleOptionImage(int category = CategoryGeneral)
        : StyleOption(OptionImage, category), scale(1, 1) {}
    Image image;
    Transform transform;
    PointF scale;

    //! Detects if the image is scaled, similar to `scale == {1, 2}` but with a dead zone.
    GX_NODISCARD bool isScaled() const;
};

struct StyleOptionRange : StyleOption {
    explicit StyleOptionRange(int category = CategoryGeneral)
        : StyleOption(OptionRange, category), progress(), hasBar(), horizontal(true) {}
    float progress;
    bool hasBar;
    bool horizontal;
};

struct StyleOptionScrollBar : StyleOption {
    explicit StyleOptionScrollBar(int category = CategoryGeneral)
        : StyleOption(OptionScrollBar, category), direction(), value(), pageStep(), minimum(),
          maximum() {}
    Direction direction;
    int value;
    int pageStep;
    int minimum;
    int maximum;
};

struct StyleOptionArc : StyleOption {
    explicit StyleOptionArc(int category = CategoryGeneral)
        : StyleOption(OptionArc, category), value(), startAngle(), stopAngle(), hasBar(), busy() {}
    float value, startAngle, stopAngle;
    bool hasBar, busy;
};

class StyleEngine {
public:
    enum PaletteRole {
        Background,
        Text,
        Frame,
        Highlight,
        Selected,
        SwitchLight,
        SwitchDark,
        SwitchThumb,
        ProgressRange,
        ProgressBar,
        ProgressThumb,
        UnknownPalette
    };

    explicit StyleEngine();
    virtual ~StyleEngine();

    virtual void paint(Painter &painter, Widget *widget, const StyleOption &option);
    GX_NODISCARD Color palette(PaletteRole role) const { return m_palettes[role]; }
    void setPalette(PaletteRole role, const Color &color);

private:
    Color m_palettes[UnknownPalette];
};
} // namespace gx
