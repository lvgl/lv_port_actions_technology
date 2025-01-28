/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_font.h"
#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_vector.h"

namespace gx {
class FontFaceMap {
public:
    FontFaceMap() GX_DEFAULT;
    ~FontFaceMap() GX_DEFAULT;

    GX_NODISCARD bool isEmpty() const { return !m_d; }
    void setBase(const FontFaceMap *base);
    GX_NODISCARD const Vector<String> *find(const String &family, int weight = Font::Regular,
                                            Font::Style style = Font::NormalStyle) const;
    GX_NODISCARD Vector<String> *modify(const String &family, int weight = Font::Regular,
                                        Font::Style style = Font::NormalStyle);
    GX_NODISCARD String mapFamily(const String &family, int weight = Font::Regular,
                                  Font::Style style = Font::NormalStyle) const;
    GX_NODISCARD bool readFile(const String &uri);
    template<class Reader, class Join> void read(Reader &reader, const Join &join);

    static bool splitFamily(Vector<String> &list, const String &family);
    GX_NODISCARD static String concatFamily(const Vector<String> &list);

private:
    struct Data;
    GX_SHARED_HELPER_DECL(Helper, Data);
    SharedRef<Data, Helper> m_d;
};

template<class Reader, class Join> void FontFaceMap::read(Reader &reader, const Join &join) {
    for (int count = reader.readU16(); count; count--) {
        String family = reader.readString();
        int weight = reader.readU8();
        int style = reader.readU8();
        Vector<String> *urls = modify(family, weight, Font::Style(style));
        for (int c = reader.readU16(); c; c--)
            urls->push_back(join(reader.readString()));
    }
}
} // namespace gx
