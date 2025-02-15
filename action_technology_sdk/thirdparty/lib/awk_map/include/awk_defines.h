#ifndef _AWK_DEFINES_H
#define _AWK_DEFINES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AWK_PAIR_DEFINE(name, K, V)         \
    typedef struct _awk_pair_##name {       \
        K key;                              \
        V value;                            \
    } awk_pair_##name;

#define AWK_DATA_MAP_DEFINE(name, K, V)     \
    AWK_PAIR_DEFINE(name, K, V);            \
    typedef struct _awk_map_##name {            \
        awk_pair_##name *data;              \
        size_t size;                        \
    } awk_map_##name;

#define AWK_DATA_LIST_DEFINE(name, type)    \
    typedef struct _awk_list_##name {            \
        type* data;                         \
        size_t size;                        \
    } awk_list_##name;                           \

AWK_PAIR_DEFINE(int32_t, int32_t, int32_t);
AWK_PAIR_DEFINE(int64_t, int64_t, int64_t);

/**
 * @brief 图像像素类型
 */
typedef enum {
    AWK_PIXEL_MODE_GREY,      //!< 8位灰度图，每一像素占用1个字节
    AWK_PIXEL_MODE_RGB_332,   //!< 8位RGB图，每一像素占用1个字节，字节顺序：高3位为红色，中3位为绿色，低2位为蓝色
    AWK_PIXEL_MODE_RGB_565,   //!< 16位RGB图，每一像素占用2个字节，字节顺序：字节0高3位绿色，低5位为蓝色；字节1低3位为绿色，高5位为红色
    AWK_PIXEL_MODE_RGB_888,   //!< 24位RGB图，每一像素占用3个字节，字节顺序：字节0为红色，字节1为绿色，字节2为蓝色
    AWK_PIXEL_MODE_ARGB_8888, //!< 32位RGB图，每一像素占用4个字节，字节顺序：字节0为Alpha, 字节1为红色，字节2为绿色，字节3为蓝色
    AWK_PIXEL_MODE_RGBA_8888, //!< 32位RGB图，每一像素占用4个字节，字节顺序：字节0为红色，字节1为绿色，字节2为蓝色，字节3为Alpha
    AWK_PIXEL_MODE_BGR_233,   //!< 8位BGR图，每一像素占用1个字节，字节顺序：高2位为蓝色，中3位为绿色，低3位为红色
    AWK_PIXEL_MODE_BGR_565,   //!< 16位BGR图，每一像素占用2个字节，字节顺序：字节0低3位为绿色，高5位为蓝色；字节1高3位为绿色，低5位为红色
    AWK_PIXEL_MODE_BGR_888,   //!< 24位BGR图，每一像素占用3个字节，字节顺序：字节0为蓝色，字节1为绿色，字节2为红色
    AWK_PIXEL_MODE_BGRA_8888, //!< 32位BGR图，每一像素占用4个字节，字节顺序：字节0为蓝色，字节1为绿色，字节2为红色，字节3为Alpha
    AWK_PIXEL_MODE_ABGR_8888, //!< 32位RGB图，每一像素占用4个字节，字节顺序：字节0为Alpha，字节1为蓝色，字节2为绿色，字节3为红色
} awk_pixel_mode_t;
    
/**
 * @brief 画布点坐标，原点是以左上角为(0,0)
 */
typedef struct _awk_point_t {
    int32_t x;          //!< x坐标
    int32_t y;          //!< y坐标
} awk_point_t;

/**
 * @brief 矩形区域定义
 */
typedef struct _awk_rect_area_t {
    int32_t x;           //!< x坐标
    int32_t y;           //!< y坐标
    int32_t width;       //!< 区域宽度, 单位：像素
    int32_t height;      //!< 区域高度, 单位：像素
} awk_rect_area_t;
    
/**
* @brief 位图定义
*/
typedef struct _awk_bitmap_t {
  awk_pixel_mode_t pixel_mode;      //!< 位图格式
  uint8_t *buffer;                  //!< 位图buffer内容
  uint32_t buffer_size;             //!< 位图buffer大小
  uint32_t width;                   //!< 位图宽度, 单位：像素
  uint32_t height;                  //!< 位图高度, 单位：像素
  uint32_t stride;                  //!< 每像素需要的字节数，如ARGB8888为4字节，RGB888为3字节，RGB565为2字节，GREY图为1字节
  bool pre_multiplied;              //!< 是否已经预乘alpha值，只有ARGB8888才有效
} awk_bitmap_t;

#ifdef __cplusplus
}
#endif

#endif
