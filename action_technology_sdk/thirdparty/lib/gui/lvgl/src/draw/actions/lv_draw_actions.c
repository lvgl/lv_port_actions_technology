/**
 * @file lv_draw_actions.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_actions.h"

#if LV_USE_ACTIONS

#include <display/display_hal.h>
#include <memory/mem_cache.h>

#if defined(CONFIG_UI_MEMORY_MANAGER)
    #include <ui_mem.h>
#endif

#include "../../lvgl.h"
#include "../../lvgl_private.h"

#if LV_USE_DRAW_VG_LITE
    #include <vg_lite/vg_lite.h>
    #include "../vg_lite/lv_draw_vg_lite.h"
#endif

#if LV_USE_DRAW_ACTS2D
    #include "lv_draw_acts2d.h"
#endif

#include "lv_draw_actsw.h"
#include "lv_acts_decoder.h"

/*********************
 *      DEFINES
 *********************/

#define ACTS_DRAW_UNIT_ID 10

#define default_handlers LV_GLOBAL_DEFAULT()->draw_buf_handlers
#define font_draw_buf_handlers LV_GLOBAL_DEFAULT()->font_draw_buf_handlers
#define image_cache_draw_buf_handlers LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_draw_acts_unit_t {
    lv_draw_unit_t base_unit;
    lv_draw_unit_t *sub_units[3];
    lv_area_t sub_areas[3];
    uint8_t sub_scores[3];
    uint8_t sub_count;
} lv_draw_acts_unit_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static inline void invalidate_area(lv_area_t *area);

static int32_t acts_draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
static int32_t acts_draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
static int32_t acts_draw_delete(lv_draw_unit_t * draw_unit);
static int32_t acts_draw_wait_for_finish(lv_draw_unit_t * draw_unit);

#ifndef _WIN32
static void draw_buf_flush_cache_cb(const lv_draw_buf_t * draw_buf, const lv_area_t * area);
static void draw_buf_clear_cb(lv_draw_buf_t * draw_buf, const lv_area_t * area);
static void draw_buf_copy_cb(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                      const lv_draw_buf_t * src, const lv_area_t * src_area);
#endif /* _WIN32 */

static void draw_buf_init_handlers(void);

#if defined(CONFIG_UI_MEMORY_MANAGER)
static void * draw_buf_malloc_cb(size_t size, lv_color_format_t color_format);
static void draw_buf_free_cb(void * buf);
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if LV_VG_LITE_USE_GPU_INIT
void gpu_init(void)
{
    vg_lite_set_command_buffer_size(CONFIG_LV_VG_LITE_COMMAND_BUFFER_SIZE << 10);
    /* FIXME:
     * Suggest max resolution is 480x480
     */
    vg_lite_init(480, 480);
}
#endif

void lv_draw_actions_init(void)
{
    lv_draw_acts_unit_t * unit = lv_draw_create_unit(sizeof(*unit));
    if(!unit) return;

    unit->base_unit.dispatch_cb = acts_draw_dispatch;
    unit->base_unit.evaluate_cb = acts_draw_evaluate;
    unit->base_unit.delete_cb = acts_draw_delete;
    unit->base_unit.wait_for_finish_cb = acts_draw_wait_for_finish;

    /* Must sort sub_units by scores from low to high (the lower, the more preferred) */

#if LV_USE_DRAW_ACTS2D
    unit->sub_units[unit->sub_count] = lv_draw_acts2d_init(true);
    unit->sub_scores[unit->sub_count++] = ACTS2D_DRAW_PREFERENCE_SCORE;
#endif

#if LV_USE_DRAW_VG_LITE
    unit->sub_units[unit->sub_count] = lv_draw_vg_lite_init(true);
    unit->sub_scores[unit->sub_count++] = VG_LITE_DRAW_PREFERENCE_SCORE;
#endif

    unit->sub_units[unit->sub_count] = lv_draw_actsw_init(true);
    unit->sub_scores[unit->sub_count++] = ACTSW_DRAW_PREFERENCE_SCORE;

    for(uint8_t i = 0; i < unit->sub_count; i++) {
        invalidate_area(&unit->sub_areas[i]);
        LV_ASSERT(unit->sub_units[i]->evaluate_cb != NULL &&
                  unit->sub_units[i]->dispatch_task_cb != NULL);
    }

    draw_buf_init_handlers();
    lv_acts_decoder_init();
}

void lv_draw_actions_deinit(void)
{
}

uint32_t lv_acts_draw_buf_width_to_stride(uint32_t w, lv_color_format_t color_format)
{
    uint32_t width_byte;

    if(color_format == LV_COLOR_FORMAT_ETC2_EAC) {
        width_byte = (w + 3) & ~3;
    }
    else {
        width_byte = (w * lv_color_format_get_bpp(color_format) + 7) >> 3; /*Round up*/
    }

    return width_byte;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline void invalidate_area(lv_area_t *area)
{
    lv_area_set(area, INT16_MAX, INT16_MAX, 0, 0);
}

static void draw_buf_init_handlers(void)
{
    lv_draw_buf_handlers_t * handlers[] = {
        &font_draw_buf_handlers,
        &default_handlers,
        &image_cache_draw_buf_handlers,
    };

    for(int i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
        handlers[i]->width_to_stride_cb = lv_acts_draw_buf_width_to_stride;

#ifndef _WIN32
        handlers[i]->flush_cache_cb = draw_buf_flush_cache_cb;
        handlers[i]->invalidate_cache_cb = draw_buf_flush_cache_cb;

        if(handlers[i] != &font_draw_buf_handlers) {
            handlers[i]->clear_cb = draw_buf_clear_cb;
            handlers[i]->copy_cb = draw_buf_copy_cb;
        }
#endif /* _WIN32 */

#if defined(CONFIG_UI_MEMORY_MANAGER)
        handlers[i]->buf_malloc_cb = draw_buf_malloc_cb;
        handlers[i]->buf_free_cb = draw_buf_free_cb;
#endif
    }
}

#ifndef _WIN32
static void draw_buf_flush_cache_cb(const lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    if(mem_is_cacheable(draw_buf->data)) {
        uint8_t bpp = lv_color_format_get_bpp(draw_buf->header.cf);
        const uint8_t *data;
        uint32_t size;

        if(area == NULL) {
            data = draw_buf->data;
            size = draw_buf->header.h * draw_buf->header.stride;
        }
        else {
            data = draw_buf->data + draw_buf->header.stride * area->y1 + area->x1 * bpp / 8;
            size = lv_area_get_height(area) * draw_buf->header.stride;
        }

        if (draw_buf->header.cf >= LV_COLOR_FORMAT_I1 && draw_buf->header.cf <= LV_COLOR_FORMAT_I8) {
            data += 4 * (1u << bpp); /* Skip the palette */
        }

        mem_dcache_flush(data, size);
        /* mem_dcache_sync(); */
    }
}

static void draw_buf_clear_cb(lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    draw_buf_flush_cache_cb(draw_buf, area);

#if LV_USE_DRAW_ACTS2D
    if(lv_draw_buf_acts2d_clear(draw_buf, area) == LV_RESULT_OK) return;
#endif

    lv_draw_buf_actsw_clear(draw_buf, area);
}

static void draw_buf_copy_cb(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                      const lv_draw_buf_t * src, const lv_area_t * src_area)
{
    draw_buf_flush_cache_cb(dest, dest_area);
    draw_buf_flush_cache_cb(src, src_area);

#if LV_USE_DRAW_ACTS2D
    if(lv_draw_buf_acts2d_copy(dest, dest_area, src, src_area) == LV_RESULT_OK) return;
#endif

    lv_draw_buf_actsw_copy(dest, dest_area, src, src_area);
}
#endif /* _WIN32 */

#if defined(CONFIG_UI_MEMORY_MANAGER)
static void * draw_buf_malloc_cb(size_t size, lv_color_format_t color_format)
{
    void * ptr = NULL;

#if CONFIG_UI_RES_MEM_POOL_SIZE > 0
    ptr = ui_mem_aligned_alloc(MEM_RES, LV_DRAW_BUF_ALIGN, size, __func__);
    if(ptr) return ptr;
#endif

#if CONFIG_UI_MEM_NUMBER_BLOCKS > 0
    if(size >= CONFIG_UI_MEM_BLOCK_SIZE) {
        ptr = ui_mem_aligned_alloc(MEM_FB, LV_DRAW_BUF_ALIGN, size, __func__);
        if(ptr) return ptr;
    }
#endif

    return lv_malloc(size + LV_DRAW_BUF_ALIGN - 1);
}

static void draw_buf_free_cb(void * buf)
{
#if CONFIG_UI_RES_MEM_POOL_SIZE > 0
    if(ui_mem_is_type(MEM_RES, buf)) {
        ui_mem_free(MEM_RES, buf);
        return;
    }
#endif

#if CONFIG_UI_MEM_NUMBER_BLOCKS > 0
    if(ui_mem_is_type(MEM_FB, buf)) {
        ui_mem_free(MEM_FB, buf);
        return;
    }
#endif

    lv_free(buf);
}
#endif /* defined(CONFIG_UI_MEMORY_MANAGER) */

static int32_t acts_draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_acts_unit_t * u = (lv_draw_acts_unit_t *)draw_unit;
    lv_draw_unit_t * target_u = NULL;
    uint8_t target_i = 0;
    int ret = LV_DRAW_UNIT_IDLE;

    /* Try to get an ready to draw. */
    lv_draw_task_t * t = lv_draw_get_next_available_task(layer, NULL, ACTS_DRAW_UNIT_ID);

    /* Return 0 is no selection, some tasks can be supported by other units. */
    if(!t || t->preferred_draw_unit_id != ACTS_DRAW_UNIT_ID) {
        acts_draw_wait_for_finish(draw_unit);
        return LV_DRAW_UNIT_IDLE;
    }

    for(uint8_t i = 0; i < u->sub_count; i++) {
        if(t->preference_score == u->sub_scores[i]) {
            target_u = u->sub_units[i];
            target_i = i;

            lv_area_t draw_area;
            lv_area_intersect(&draw_area, &t->clip_area, &t->_real_area);
            lv_area_join(&u->sub_areas[i], &u->sub_areas[i], &draw_area);
            break;
        }
    }

    if(target_u) {
        for(uint8_t i = 0; i < u->sub_count; i++) {
            if(i != target_i && lv_area_is_on(&u->sub_areas[i], &u->sub_areas[target_i])) {
                if(u->sub_units[i]->wait_for_finish_cb) {
                    u->sub_units[i]->wait_for_finish_cb(u->sub_units[i]);
                    invalidate_area(&u->sub_areas[i]);
                }
            }
        }

        ret = target_u->dispatch_task_cb(target_u, layer, t);
        LV_LOG_TRACE("execute acts unit[%u] (ret=%d)", target_i, ret);

        if(ret > 0) {
            /* Request a new dispatching as it can get a new task. */
            lv_draw_dispatch_request();
        }
    }

    return ret;
}

static int32_t acts_draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    lv_draw_acts_unit_t * u = (lv_draw_acts_unit_t *)draw_unit;
    uint8_t score = task->preference_score;

    for(uint8_t i = 0; i < u->sub_count; i++) {
        if(!u->sub_units[i]->evaluate_cb)
            continue;

        u->sub_units[i]->evaluate_cb(u->sub_units[i], task);

        if(task->preference_score < score) {
            task->preferred_draw_unit_id = ACTS_DRAW_UNIT_ID;
            LV_LOG_TRACE("select acts unit[%u] task %u, score %u", i, task->type, task->preference_score);
            return 1;
        }
    }

    LV_LOG_TRACE("unsupported task %u", task->type);
    return 0;
}

static int32_t acts_draw_delete(lv_draw_unit_t * draw_unit)
{
    lv_draw_acts_unit_t * u = (lv_draw_acts_unit_t *)draw_unit;

    for(uint8_t i = 0; i < u->sub_count; i++) {
        if(u->sub_units[i]->delete_cb)
            u->sub_units[i]->delete_cb(u->sub_units[i]);
    }

    return 1;
}

static int32_t acts_draw_wait_for_finish(lv_draw_unit_t * draw_unit)
{
    lv_draw_acts_unit_t * u = (lv_draw_acts_unit_t *)draw_unit;

    for(uint8_t i = 0; i < u->sub_count; i++) {
        if(u->sub_units[i]->wait_for_finish_cb) {
            u->sub_units[i]->wait_for_finish_cb(u->sub_units[i]);
            invalidate_area(&u->sub_areas[i]);
        }
    }

    return 1;
}

#endif /*LV_USE_ACTIONS*/
