/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once
#include "gx_color.h"
#include "gx_geometry.h"
#include "gx_image.h"
#include "gx_jsvm.h"
#include "gx_maputils.h"
#include "gx_metadefs.h"
#include "gx_signals.h"
#include "gx_string.h"
#include "gx_vector.h"

namespace gx {

class NavPoint {
public:
    NavPoint(float lat, float lng);
    ~NavPoint() GX_DEFAULT;

    float mLatitude;
    float mLongitude;
    int x;
    int y;
    bool isCached;
    bool needRender;
    Point mOffset;
};

/**
 * 当前位置图标管理类
 */
class NavArrow {
public:
    NavArrow();
    ~NavArrow() GX_DEFAULT;

    void setArrowImg(const String &path);

    // 当前位置经纬度
    PointF coordinate;
    Image arrowImg;
    // 瓦片序号
    Point mTilePos;
    // 相对瓦片偏移量
    Point mTileOffset;
    // 是否需要绘制
    bool visible{false};
    // 在屏幕上的绘制坐标
    Point pos;

    /**
     * 导航路线信息： 宽度，前景色，背景色
     */
    int mLineWidth;
    Color mLineBackgroundColor;
    Color mLineForgeColor;
};

class MapTile {
public:
    MapTile();
    ~MapTile() GX_DEFAULT;
    Point pos; // Tiles at the coordinate points on the screen
    /**
     * Tile serial number
     */
    int tileX;
    int tileY;

    Image img;
};

class MapTileManager {
public:
    MapTileManager();
    ~MapTileManager();

    void updatePosition(float lat, float lng);
    void updatePosition(const PointF &position) {
        updatePosition(position.x(), position.y());
        loadTile();
    }
    void setBaseUri(const String &path) { m_baseUri = path; }
    const String &baseUri() const { return m_baseUri; }
    void setTileType(int type) {
        m_tileType = type;
        loadTile();
    }
    int tileType() const { return m_tileType; }
    void setLoadPlace(const String &path) { m_loadingPlace = path; };
    const String &loadPlace() { return m_loadingPlace; };
    PointF coordinate() const { return PointF(mLatitude, mLongitude); }
    const JsValue &missTiles() const { return m_missTiles; }
    const JsValue &directionInfo() const { return mDirectionInfo; }
    void setLevel(int level) {
        mLevel = level;
        mMaxTileX = (1 << mLevel) - 1;
        if (!m_baseUri.empty()) {
            loadTile();
        }
    }
    int level() const { return mLevel; }

    void setArrowPosition(const PointF &position) {
        navArrow.coordinate = position;
        calcDirection();
        loadTile();
    };
    PointF arrowPosition() const { return navArrow.coordinate; }

    void setArrowIcon(const String &path) { navArrow.setArrowImg(path); }
    String arrowIcon() const { return navArrow.arrowImg.uri(); };

    void setArrowLineWidth(int w) { navArrow.mLineWidth = w; }
    int arrowLineWidth() const { return navArrow.mLineWidth; };

    void setArrowLineBackgroundColor(const Color &color) { navArrow.mLineBackgroundColor = color; }
    const Color &arrowLineBackgroundColor() const { return navArrow.mLineBackgroundColor; }

    void setArrowLineForgeColor(const Color &color) { navArrow.mLineForgeColor = color; }
    const Color &arrowLineForgeColor() const { return navArrow.mLineForgeColor; }

    void setSmallMem(bool small) {
        mSmallMem = small;
        // 高端设备不需要修改以下属性
        if (!small)
            return;
        mTileSize = 512;
        mColSize = 2;
        mRowSize = 2;
        /**
         * 低内存设备中，将四张大小 256 的瓦片缩放至 512 进行绘制
         * 将一张瓦片以瓦片中心作为原点建立直角坐标系，分为四象限
         * 当屏幕中心点跨越瓦片象限时，需要重新加载瓦片
         */
        mScaleLevel = 2.0f;

        mMoveMaxBoundary = mTileSize / (mScaleLevel * 2);
        mMoveMinBoundary = mTileSize / (mScaleLevel * 2);
        MapUtil::setTileSize(mTileSize);
    }
    bool smallMem() { return mSmallMem; }

    Signal<> missTile;
    Signal<> directionUpdate;

protected:
    // The latitude and longitude of the coordinates of the center point of the screen, which
    // changes when you slide the map
    float mLatitude;
    float mLongitude;

    /**
     * The serial number of the central tile,
     * from which the serial number of the other tiles is calculated
     */
    int mCenterTileX{};
    int mCenterTileY{};

    /**
     * The size of the individual tiles
     */
    int mTileSize;

    // Whether it is a low-memory device
    bool mSmallMem;

    // When you slide the map, update the boundary value of the map tile
    int mMoveMaxBoundary;
    int mMoveMinBoundary;

    float mScaleLevel{1.0f};

    /**
     * Zoom level
     */
    int mLevel;

    /**
     * Array of tile information
     */
    Vector<MapTile> mTiles;

    /**
     * 当前经纬度在屏幕绘制坐标，相对于自身瓦片边缘的偏差值
     */
    int16_t mCenterOffsetX;
    int16_t mCenterOffsetY;

    // 当前坐标点对象
    NavArrow navArrow;

    // 导航路线坐标点集合
    Vector<NavPoint> navPoints;

    // 导航线每一段起止点
    Vector<NavPoint> navLinePoint;

    /**
     * Load the tiles based on the current latitude and longitude
     */
    void loadTile();

    /**
     * Set the component size
     * @param w
     * @param h
     */
    void setWidgetSize(int w, int h);

    /**
     * Move all tiles
     * @param offsetX
     * @param offsetY
     */
    bool moveTiles(int offsetX, int offsetY);
    bool moveTiles(const Point &offset) { return moveTiles(offset.x(), offset.y()); }

    void navPointsUpdate();

    Point footPoint;
    int footPointIndex{0};

    JsValue mDirectionInfo;

private:
    /**
     * Obtain the tile file path based on the tile serial number
     * @param tileX Tile transverse serial number
     * @param tileY The longitudinal serial number of the tile
     * @param level Zoom level
     * @return
     */
    String getFilePath(int tileX, int tileY, int level);

    // 计算当前位置离哪一段线最近
    void calcDirection();

    /**
     * 组件宽高
     */
    int mWidgetWidth;
    int mWidgetHeight;
    /**
     * 每行/列 瓦片数量
     */
    int mColSize;
    int mRowSize;

    JsValue m_missTiles;

    // 下载瓦片的存储目录
    String m_baseUri;
    // 正在加载的占位瓦片图
    String m_loadingPlace;

    int m_tileType{0};

    /**
     * 瓦片最大和最小的 x 序号
     */
    int mMinTileX;
    int mMaxTileX;
    Vector<String> mExistTile;
};
} // namespace gx
