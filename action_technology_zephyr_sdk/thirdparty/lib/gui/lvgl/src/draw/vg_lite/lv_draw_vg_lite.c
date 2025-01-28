/**
 * @file lv_vg_lite_draw.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_vg_lite.h"

#if LV_USE_DRAW_VG_LITE

#include "../lv_draw_private.h"
#include "lv_draw_vg_lite_type.h"
#include "lv_vg_lite_path.h"
#include "lv_vg_lite_utils.h"
#include "lv_vg_lite_decoder.h"
#include "lv_vg_lite_grad.h"
#include "lv_vg_lite_pending.h"
#include "lv_vg_lite_stroke.h"

/*********************
 *      DEFINES
 *********************/

#define VG_LITE_DRAW_UNIT_ID 2

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int32_t draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);

static int32_t draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);

static int32_t draw_delete(lv_draw_unit_t * draw_unit);

static int32_t draw_wait_for_finish(lv_draw_unit_t * draw_unit);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_vg_lite_init(void)
{
#if LV_VG_LITE_USE_GPU_INIT
    extern void gpu_init(void);
    static bool inited = false;
    if(!inited) {
        gpu_init();
        inited = true;
    }
#endif

    lv_vg_lite_dump_info();

    /* lv_draw_buf_vg_lite_init_handlers(); */

    lv_draw_vg_lite_unit_t * unit = lv_draw_create_unit(sizeof(lv_draw_vg_lite_unit_t));
    unit->base_unit.dispatch_cb = draw_dispatch;
    unit->base_unit.evaluate_cb = draw_evaluate;
    unit->base_unit.delete_cb = draw_delete;
    unit->base_unit.wait_for_finish_cb = draw_wait_for_finish;

    lv_vg_lite_image_dsc_init(unit);
#if LV_USE_VECTOR_GRAPHIC
    lv_vg_lite_grad_init(unit, LV_VG_LITE_GRAD_CACHE_CNT);
    lv_vg_lite_stroke_init(unit, LV_VG_LITE_STROKE_CACHE_CNT);
#endif
    lv_vg_lite_path_init(unit);
    /* lv_vg_lite_decoder_init(); */
}

void lv_draw_vg_lite_deinit(void)
{
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#define SPI0_BASE_ADDR 0x10000000
#define SPI0_BASE_END_ADDR 0x14000000
#define buf_is_nor(buf) \
        ((((uint32_t)buf) >= SPI0_BASE_ADDR) && (((uint32_t)buf) < SPI0_BASE_END_ADDR))

static inline bool is_vglite_accessible(const void * ptr)
{
    return (buf_is_nor(ptr) || ((uintptr_t)ptr & (LV_DRAW_BUF_ALIGN - 1))) ? false : true;
}

static bool check_image_is_supported(const lv_draw_image_dsc_t * dsc)
{
    lv_image_src_t src_type = lv_image_src_get_type(dsc->src);
    if(src_type == LV_IMAGE_SRC_VARIABLE) {
        lv_image_dsc_t * image = (lv_image_dsc_t *)dsc->src;
        if(image->data && !is_vglite_accessible(image->data)) {
            LV_LOG_TRACE("image data not accessible");
            return false;
        }
    }

    return lv_vg_lite_is_src_cf_supported(dsc->header.cf);
}

#if LV_USE_FREETYPE == 0

static bool check_font_is_supported(const lv_draw_label_dsc_t * dsc)
{
    const lv_font_fmt_txt_dsc_t * font_dsc = dsc->font->dsc;
    if(!font_dsc) return true;
    if(!(font_dsc->bpp == 0 || font_dsc->bpp == LV_FONT_GLYPH_FORMAT_VECTOR ||
         font_dsc->bpp == LV_FONT_GLYPH_FORMAT_IMAGE)) {
        LV_LOG_TRACE("only support image font and vector font");
        return false;
    }
    return true;
}

#endif /* LV_USE_FREETYPE == 0 */

static bool check_blend_is_supported(uint8_t blend_mode)
{
    return lv_vg_lite_blend_mode(blend_mode, false) != VG_LITE_BLEND_NONE;
}

static void lv_vg_lite_target_buffer_invalidate(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_area_t draw_area = {
        draw_unit->clip_area->x1 - layer->buf_area.x1,
        draw_unit->clip_area->y1 - layer->buf_area.y1,
        draw_unit->clip_area->x2 - layer->buf_area.x1,
        draw_unit->clip_area->y2 - layer->buf_area.y1,
    };

    lv_draw_buf_invalidate_cache(layer->draw_buf, &draw_area);
}

static void draw_execute(lv_draw_vg_lite_unit_t * u)
{
    lv_draw_task_t * t = u->task_act;
    lv_draw_unit_t * draw_unit = (lv_draw_unit_t *)u;

    lv_layer_t * layer = u->base_unit.target_layer;

    lv_vg_lite_target_buffer_invalidate(draw_unit, layer);

    lv_vg_lite_buffer_from_draw_buf(&u->target_buffer, layer->draw_buf);

    /* VG-Lite will output premultiplied image, set the flag correspondingly. */
    /* lv_draw_buf_set_flag(layer->draw_buf, LV_IMAGE_FLAGS_PREMULTIPLIED); */

    vg_lite_identity(&u->global_matrix);
    if(layer->buf_area.x1 || layer->buf_area.y1) {
        vg_lite_translate(-layer->buf_area.x1, -layer->buf_area.y1, &u->global_matrix);
    }

#if LV_DRAW_TRANSFORM_USE_MATRIX
    vg_lite_matrix_t layer_matrix;
    lv_vg_lite_matrix(&layer_matrix, &t->matrix);
    lv_vg_lite_matrix_multiply(&u->global_matrix, &layer_matrix);

    /* Crop out extra pixels drawn due to scaling accuracy issues */
    if(vg_lite_query_feature(gcFEATURE_BIT_VG_SCISSOR)) {
        lv_area_t scissor_area = layer->phy_clip_area;
        lv_area_move(&scissor_area, -layer->buf_area.x1, -layer->buf_area.y1);
        lv_vg_lite_set_scissor_area(&scissor_area);
    }
#endif

    switch(t->type) {
        case LV_DRAW_TASK_TYPE_LABEL:
            lv_draw_vg_lite_label(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_FILL:
            lv_draw_vg_lite_fill(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_BORDER:
            lv_draw_vg_lite_border(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_BOX_SHADOW:
            lv_draw_vg_lite_box_shadow(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_IMAGE:
            lv_draw_vg_lite_img(draw_unit, t->draw_dsc, &t->area, false);
            break;
        case LV_DRAW_TASK_TYPE_ARC:
            lv_draw_vg_lite_arc(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_LINE:
            lv_draw_vg_lite_line(draw_unit, t->draw_dsc);
            break;
        case LV_DRAW_TASK_TYPE_LAYER:
            lv_draw_vg_lite_layer(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_TRIANGLE:
            lv_draw_vg_lite_triangle(draw_unit, t->draw_dsc);
            break;
        case LV_DRAW_TASK_TYPE_MASK_RECTANGLE:
            lv_draw_vg_lite_mask_rect(draw_unit, t->draw_dsc, &t->area);
            break;
#if LV_USE_VECTOR_GRAPHIC
        case LV_DRAW_TASK_TYPE_VECTOR:
            lv_draw_vg_lite_vector(draw_unit, t->draw_dsc);
            break;
#endif
        case LV_DRAW_TASK_TYPE_VECTOR_CUSTOM: {
                lv_draw_vector_custom_dsc_t * dsc = lv_draw_task_get_vector_custom_dsc(t);
                if(dsc->exec_cb)
                    dsc->exec_cb(t, layer, dsc->user_data);
            }
            break;
        default:
            break;
    }

    lv_vg_lite_flush(u);
}

static int32_t draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_vg_lite_unit_t * u = (lv_draw_vg_lite_unit_t *)draw_unit;

    /* Return immediately if it's busy with draw task. */
    if(u->task_act) {
        return 0;
    }

    /* Try to get an ready to draw. */
    lv_draw_task_t * t = lv_draw_get_next_available_task(layer, NULL, VG_LITE_DRAW_UNIT_ID);

    /* Return 0 is no selection, some tasks can be supported by other units. */
    if(!t || t->preferred_draw_unit_id != VG_LITE_DRAW_UNIT_ID) {
        lv_vg_lite_finish(u);
        return LV_DRAW_UNIT_IDLE;
    }

    /* Return if target buffer format is not supported. */
    if(!lv_vg_lite_is_dest_cf_supported(layer->color_format)) {
        return LV_DRAW_UNIT_IDLE;
    }

    void * buf = lv_draw_layer_alloc_buf(layer);
    if(!buf) {
        return LV_DRAW_UNIT_IDLE;
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    u->base_unit.target_layer = layer;
    u->base_unit.clip_area = &t->clip_area;
    u->task_act = t;

    draw_execute(u);

    u->task_act->state = LV_DRAW_TASK_STATE_READY;
    u->task_act = NULL;

    /*The draw unit is free now. Request a new dispatching as it can get a new task*/
    lv_draw_dispatch_request();

    return 1;
}

static int32_t draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);

    /* Return if target buffer format is not supported. */
    const lv_draw_dsc_base_t * base_dsc = task->draw_dsc;
    if(!lv_vg_lite_is_dest_cf_supported(base_dsc->layer->color_format)) {
        return -1;
    }

    switch(task->type) {
        case LV_DRAW_TASK_TYPE_FILL:
        case LV_DRAW_TASK_TYPE_BORDER:
#if LV_VG_LITE_USE_BOX_SHADOW
        case LV_DRAW_TASK_TYPE_BOX_SHADOW:
#endif
        case LV_DRAW_TASK_TYPE_ARC:
        case LV_DRAW_TASK_TYPE_TRIANGLE:
        case LV_DRAW_TASK_TYPE_MASK_RECTANGLE:
        case LV_DRAW_TASK_TYPE_VECTOR_CUSTOM:

#if LV_USE_VECTOR_GRAPHIC
        case LV_DRAW_TASK_TYPE_VECTOR:
#endif
            break;

        case LV_DRAW_TASK_TYPE_LINE: {
                lv_draw_line_dsc_t * draw_dsc = task->draw_dsc;
                if (!check_blend_is_supported(draw_dsc->blend_mode)) {
                    return 0;
                }
            }
            break;

        case LV_DRAW_TASK_TYPE_LAYER: {
                const lv_draw_image_dsc_t * draw_dsc = task->draw_dsc;
                if (!check_blend_is_supported(draw_dsc->blend_mode)) {
                    return 0;
                }
            }
            break;

        case LV_DRAW_TASK_TYPE_IMAGE: {
                const lv_draw_image_dsc_t * draw_dsc = task->draw_dsc;
                if (!check_blend_is_supported(draw_dsc->blend_mode) ||
                    !check_image_is_supported(draw_dsc)) {
                    return 0;
                }
            }
            break;

        case LV_DRAW_TASK_TYPE_LABEL: {
                lv_draw_label_dsc_t * draw_dsc = task->draw_dsc;
                if (!check_blend_is_supported(draw_dsc->blend_mode)) {
                    return 0;
                }

#if LV_USE_FREETYPE == 0
                if (!check_font_is_supported(task->draw_dsc)) {
                    return 0;
                }
#endif
            }
            break;

        default:
            /*The draw unit is not able to draw this task. */
            return 0;
    }

    /* The draw unit is able to draw this task. */
    task->preference_score = 80;
    task->preferred_draw_unit_id = VG_LITE_DRAW_UNIT_ID;
    return 1;
}

static int32_t draw_delete(lv_draw_unit_t * draw_unit)
{
    lv_draw_vg_lite_unit_t * unit = (lv_draw_vg_lite_unit_t *)draw_unit;

    lv_vg_lite_image_dsc_deinit(unit);
#if LV_USE_VECTOR_GRAPHIC
    lv_vg_lite_grad_deinit(unit);
    lv_vg_lite_stroke_deinit(unit);
#endif
    lv_vg_lite_path_deinit(unit);
    /* lv_vg_lite_decoder_deinit(); */
    return 1;
}

static int32_t draw_wait_for_finish(lv_draw_unit_t * draw_unit)
{
    vg_lite_finish();
    return 0;
}

#endif /*LV_USE_DRAW_VG_LITE*/
