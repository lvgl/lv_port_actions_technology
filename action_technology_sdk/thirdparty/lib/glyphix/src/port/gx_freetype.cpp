/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_freetype.h"
#include "gx_file.h"

#include "freetype/freetype.h"
#include "freetype/internal/sfnt.h"
#include FT_OUTLINE_H

#define STREAM_FILE(stream) ((File *)(stream)->descriptor.pointer)
#define FT_LOAD_FLAGS       (FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE)

namespace gx {
static unsigned long streamRead(FT_Stream stream, unsigned long offset, unsigned char *buffer,
                                unsigned long count) {
    if (!count && offset > stream->size)
        return 1;

    File *file = STREAM_FILE(stream);

    if (stream->pos != offset)
        file->seek(File::off_t(offset));
    if (count == 0) {
        File::off_t res = file->seek(File::off_t(offset));
        if (res < 0)
            return res;
        return 0;
    }

    GX_ASSERT(buffer);
    return static_cast<unsigned long>(file->read(buffer, count));
}

static void streamClose(FT_Stream stream) {
    File *file = STREAM_FILE(stream);
    GX_ASSERT(file != nullptr);
    delete file;

    stream->descriptor.pointer = nullptr;
    stream->size = 0;
    stream->base = nullptr;
}

static bool streamOpen(FT_Stream stream, const String &path) {
    stream->descriptor.pointer = nullptr;
    stream->pathname.pointer = nullptr;
    stream->base = nullptr;
    stream->pos = 0;
    stream->read = nullptr;
    stream->close = nullptr;

    File file(path);
    if (!file.open())
        return false;

    stream->size = file.size();
    if (!stream->size)
        return false;

    File *fp = new File;
    fp->swap(file);
    stream->descriptor.pointer = fp;
    stream->read = streamRead;
    stream->close = streamClose;
    return true;
}

static void ttHMetrics(TT_Face face, FT_UInt idx, FT_Short *lsb, FT_UShort *aw) {
    ((SFNT_Service)face->sfnt)->get_metrics(face, 0, idx, lsb, aw);
}

static int outlineMoveTo(const FT_Vector *to, void *user) {
    TrueTypeStroker *s = static_cast<TrueTypeStroker *>(user);
    s->moveTo(to->x, to->y);
    return 0;
}

static int outlineLineTo(const FT_Vector *to, void *user) {
    TrueTypeStroker *s = static_cast<TrueTypeStroker *>(user);
    s->lineTo(to->x, to->y);
    return 0;
}

static int outlineConicTo(const FT_Vector *control, const FT_Vector *to, void *user) {
    TrueTypeStroker *s = static_cast<TrueTypeStroker *>(user);
    s->conicTo(control->x, control->y, to->x, to->y);
    return 0;
}

static int outlineCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to,
                          void *user) {
    TrueTypeStroker *s = static_cast<TrueTypeStroker *>(user);
    s->cubicTo(control1->x, control1->y, control2->x, control2->y, to->x, to->y);
    return 0;
}

static FT_Outline_Funcs outlineFuncs = {
    outlineMoveTo, outlineLineTo, outlineConicTo, outlineCubicTo, 0, 0};

FreeTypeLibrary::FreeTypeLibrary() {
    FT_Error err = FT_Init_FreeType(&m_library);
    GX_ASSERT(err == 0);
    GX_UNUSED(err);
}

FreeTypeLibrary::~FreeTypeLibrary() { FT_Done_FreeType(m_library); }

TrueTypeFace *FreeTypeLibrary::openFace(const String &faceName) {
    FT_Face face;
    FT_Open_Args args;
    args.flags = FT_OPEN_STREAM;
    args.stream = new FT_StreamRec;
    streamOpen(args.stream, faceName);
    FT_Error error = FT_Open_Face(m_library, &args, 0, &face);
    if (error) {
        delete args.stream;
        return nullptr;
    }
    return new FreeTypeFace(face);
}

FreeTypeFace::~FreeTypeFace() {
    FT_Stream stream = face()->stream;
    FT_Done_Face(face());
    delete stream;
}

static void loadHoriMetrics(FT_Face face, FT_UInt index) {
    FT_Short leftBearing = 0;
    FT_UShort advanceWidth = 0;
    ttHMetrics((TT_Face)face, index, &leftBearing, &advanceWidth);
    face->glyph->metrics.horiBearingX = leftBearing;
    face->glyph->metrics.horiAdvance = advanceWidth;
}

bool FreeTypeFace::loadChar(uint32_t code, LoadFlags flags) {
    FT_UInt index = FT_Get_Char_Index(face(), code);
    if (index > 0) {
        if (flags & OnlyMetrics) {
            loadHoriMetrics(face(), index);
            return true;
        }
        FT_Load_Glyph(face(), index, FT_LOAD_FLAGS);
        if (code == ' ')
            loadHoriMetrics(face(), index);
        return face()->glyph != nullptr;
    }
    return false;
}

bool FreeTypeFace::glyphOutline(TrueTypeStroker *stroker) {
    FT_Error err = FT_Outline_Decompose(&face()->glyph->outline, &outlineFuncs, stroker);
    return err == FT_Err_Ok;
}

void FreeTypeFace::glyphMetrics(TrueTypeGlyphMetrics *metrics) {
    FT_Glyph_Metrics &m = face()->glyph->metrics;
    metrics->top = int(m.horiBearingY);
    metrics->left = int(m.horiBearingX);
    metrics->width = int(m.width);
    metrics->height = int(m.height);
    metrics->xadvance = int(m.horiAdvance);
    metrics->yadvance = int(m.vertAdvance);
}

int FreeTypeFace::unitsPerEM() const { return face()->units_per_EM; }

int FreeTypeFace::ascender() const { return face()->ascender; }

int FreeTypeFace::descender() const { return face()->descender; }
} // namespace gx
