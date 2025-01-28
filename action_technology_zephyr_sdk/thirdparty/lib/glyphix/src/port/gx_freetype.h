/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "loader/gx_fontloader_ttf.h"

struct FT_FaceRec_;
struct FT_LibraryRec_;

namespace gx {
class FreeTypeFace : public TrueTypeFace {
public:
    FreeTypeFace(FT_FaceRec_ *face) : m_face(face) {}
    virtual ~FreeTypeFace();
    FT_FaceRec_ *face() const { return m_face; }
    virtual bool loadChar(uint32_t code, LoadFlags flags);
    virtual bool glyphOutline(TrueTypeStroker *stroker);
    virtual void glyphMetrics(TrueTypeGlyphMetrics *metrics);
    virtual int unitsPerEM() const;
    virtual int ascender() const;
    virtual int descender() const;

private:
    FT_FaceRec_ *m_face;
};

class FreeTypeLibrary : public TrueTypeLibrary {
public:
    FreeTypeLibrary();
    ~FreeTypeLibrary();
    virtual TrueTypeFace *openFace(const String &faceName);

private:
    FT_LibraryRec_ *m_library;
};
} // namespace gx
