/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_font.h"
#include "gx_vector.h"

namespace gx {
class Texture;

/**
 * @class TextLayout
 *
 * @brief The TextLayout class represents the layout of a text string with specific font, flags,
 * line height, and indentation.
 *
 * The TextLayout class is used to calculate the layout of a text string with specific font
 * settings, flags, line height, and indentation. It provides methods to calculate the size and
 * position of the individual text lines, as well as the overall size of the text layout. The
 * layout can be adjusted based on various formatting options, such as text wrapping and
 * eliding.
 *
 * @see Font
 */
class TextLayout {
public:
    struct LineInfo {
        int offset; // offset of the first character of the line
        // ....... CHAR ... CHAR ... CHAR .......
        //   ^           ^
        //   left        space  (Q26.6 pixels)
        int left, space;
        LineInfo() : offset(), left(), space() {}
        LineInfo(int offset, int left, int space) : offset(offset), left(left), space(space) {}
    };
    class Render;

    TextLayout();
    explicit TextLayout(const Font &font, int flags = 0, int lineHeight = 0);
    void setup(const Font &font, int flags = 0, int lineHeight = 0);
    /**
     * Specifies the first-line indent space when text is laid out.
     * @param indent Indented space with FP26.6 format.
     */
    void setIndent(int indent) { m_indent = indent; }
    void layoutRightToLeft() {}
    /**
     * @brief Layout the given text within the specified number of lines and rectangle.
     *
     * @param text The input text.
     * @param lines The maximum number of lines to fit the text into. Pass 0 for unlimited lines.
     * @param box The rectangle box size to fit the text into.
     * @return The width of the last line with FP26.6 format.
     */
    int layout(const String &text, int lines, const Size &box);
    //! Get the character information for each line after the layout.
    GX_NODISCARD const Vector<LineInfo> &lines() const { return m_lines; }
    //! Get whether the layout is truncated (including elided).
    GX_NODISCARD bool isTruncated() const { return m_truncated; }
    //! Get whether the layout is elided.
    GX_NODISCARD bool isElided() const { return m_elided; }
    //! Get the pixel line height for the FP26Q6 fixed-point number.
    GX_NODISCARD int lineHeight() const { return m_lineHeight; }
    //! Get the text flags.
    GX_NODISCARD int textFlags() const { return m_flags; }
    //! Get the layout text.
    GX_NODISCARD const String &text() const { return m_text; }
    GX_NODISCARD const Font &font() const { return m_font; }

private:
    struct WrapState;
    void scanNextLine(WrapState &state, int maxWidth);
    void scanLastLine(WrapState &state, int maxWidth);
    bool scanNextWord(WrapState &state, int maxWidth);
    void pushLine(int offset, int width, int maxWidth);

private:
    String m_text;
    Font m_font;
    uint16_t m_lineHeight;
    uint16_t m_flags;
    int m_indent;
    bool m_elided;
    bool m_truncated;
    Vector<LineInfo> m_lines;
};
} // namespace gx
