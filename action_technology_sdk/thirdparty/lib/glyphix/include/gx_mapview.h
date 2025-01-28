/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include "gx_jsvm.h"
#include "gx_maptile.h"
#include "gx_vectorpath.h"
#include "gx_widget.h"

namespace gx {
class MapView : public Widget, public MapTileManager {
    GX_OBJECT

public:
    explicit MapView(Widget *parent = nullptr);
    virtual ~MapView();

    void setLevel(int level) {
        MapTileManager::setLevel(level);
        update();
    }

    void setArrowPosition(const PointF &position) {
        MapTileManager::setArrowPosition(position);
        update();
    };

    GX_PROPERTY(JsValue missTiles, get missTiles, signal missTile)
    GX_PROPERTY(JsValue directionInfo, get directionInfo, signal directionUpdate);
    GX_PROPERTY(String baseUri, get baseUri, set setBaseUri, uri true)
    GX_PROPERTY(int tileType, get tileType, set setTileType)
    GX_PROPERTY(String loadPlace, get loadPlace, set setLoadPlace, uri true);
    GX_PROPERTY(int zoomLevel, get level, set setLevel);
    GX_PROPERTY(String arrowIcon, get arrowIcon, set setArrowIcon, uri true);
    GX_PROPERTY(PointF navCoordinate, set setArrowPosition, get arrowPosition);
    GX_PROPERTY(int arrowLineWidth, set setArrowLineWidth, get arrowLineWidth);
    GX_PROPERTY(Color arrowLineBackgroundColor, set setArrowLineBackgroundColor,
                get arrowLineBackgroundColor);
    GX_PROPERTY(Color arrowLineForgeColor, set setArrowLineForgeColor, get arrowLineForgeColor);
    GX_PROPERTY(bool smallMem, get smallMem, set setSmallMem);

    GX_METHOD void reload();
    GX_METHOD void locate(); // 回到当前位置
    GX_METHOD void insetNavPoint(const JsValue &linePoints);
    GX_METHOD void startNav(const gx::JsValue &linePoints);

protected:
    virtual void resizeEvent(ResizeEvent *event);
    virtual void paintEvent(PaintEvent *event);
    virtual bool gestureEvent(GestureEvent *event);

private:
    /**
     * 绘制当前点的图标
     */
    void drawArrowIcon(Painter p);

    /**
     * 绘制导航路线
     * @param p
     */
    void drawNavPath(Painter &p);

    void getFooterPoint();

    void drawCompletePath(gx::Painter &p, VectorPath path);
    void drawRemainPath(gx::Painter &p, VectorPath path);
};
} // namespace gx
