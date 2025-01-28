/**
 * @file lv_vg_lite_decoder.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lvgl_decoder.h"

#include <stdlib.h>
#include <string.h>
#include "../../src/core/lv_global.h"
#include "../../src/draw/lv_image_decoder_private.h"
#include "../../src/libs/bin_decoder/lv_bin_decoder.h"

/*********************
 *      DEFINES
 *********************/

#define DECODER_NAME    "ACTS_IMGDEC"

#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header);
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lvgl_decoder_init(void)
{
    lv_image_decoder_t * decoder = lv_image_decoder_create();
    if(decoder) {
        lv_image_decoder_set_info_cb(decoder, decoder_info);
        lv_image_decoder_set_open_cb(decoder, decoder_open);
        lv_image_decoder_set_close_cb(decoder, decoder_close);

        decoder->name = DECODER_NAME;
    }
}

void lvgl_decoder_deinit(void)
{
    lv_image_decoder_t * decoder = NULL;
    while((decoder = lv_image_decoder_get_next(decoder)) != NULL) {
        if(decoder->info_cb == decoder_info) {
            lv_image_decoder_delete(decoder);
            break;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header)
{
    if(lv_bin_decoder_info(decoder, dsc, header) != LV_RESULT_OK)
        return LV_RESULT_INVALID;

    if(header->flags & LV_IMAGE_FLAGS_COMPRESSED)
        return LV_RESULT_INVALID;

    if(!LV_COLOR_FORMAT_IS_ALPHA_ONLY(header->cf))
        return LV_RESULT_INVALID;

    return LV_RESULT_OK;
}

static lv_result_t decoder_open_variable(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/

    dsc->decoded = lv_malloc(sizeof(lv_draw_buf_t));
    LV_ASSERT_MALLOC(dsc->decoded);
    if(dsc->decoded == NULL) return LV_RESULT_INVALID;

    lv_draw_buf_from_image((lv_draw_buf_t *)dsc->decoded, dsc->src);
    return LV_RESULT_OK;
}

static lv_result_t decoder_open_file(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/

    const char * path = dsc->src;
    uint32_t buf_len = dsc->header.stride * dsc->header.h;

    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, path, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        LV_LOG_ERROR("open %s failed", path);
        return LV_RESULT_INVALID;
    }

    res = lv_fs_seek(&file, sizeof(lv_image_header_t), LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        LV_LOG_WARN("etc2 file seek %s failed", path);
        lv_fs_close(&file);
        return LV_RESULT_INVALID;
    }

    lv_draw_buf_t *decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers,
                                dsc->header.w, dsc->header.h, dsc->header.cf, dsc->header.stride);
    if(decoded == NULL) {
        lv_fs_close(&file);
        return LV_RESULT_INVALID;
    }

    uint32_t rn;
    res = lv_fs_read(&file, decoded->data, buf_len, &rn);
    if(res != LV_FS_RES_OK || rn != buf_len) {
        LV_LOG_WARN("Read header failed: %d", res);
        lv_draw_buf_destroy(decoded);
        lv_fs_close(&file);
        return LV_RESULT_INVALID;
    }

    lv_fs_close(&file);
    dsc->decoded = decoded;
    return LV_RESULT_OK;
}

static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    lv_result_t res = LV_RESULT_INVALID;

    switch(dsc->src_type) {
        case LV_IMAGE_SRC_VARIABLE:
            return decoder_open_variable(decoder, dsc);
        case LV_IMAGE_SRC_FILE:
            res = decoder_open_file(decoder, dsc);
            break;
        default:
            return LV_RESULT_INVALID;
    }

    if(dsc->args.no_cache) return res;

    /*If the image cache is disabled, just return the decoded image*/
    if(!lv_image_cache_is_enabled()) return res;

    /*Add the decoded image to the cache*/
    if(res == LV_RESULT_OK) {
        lv_image_cache_data_t search_key;
        search_key.src_type = dsc->src_type;
        search_key.src = dsc->src;
        search_key.slot.size = dsc->decoded->data_size;

        lv_cache_entry_t * entry = lv_image_decoder_add_to_cache(decoder, &search_key, dsc->decoded, NULL);
        if(entry == NULL) {
            lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);
            dsc->decoded = NULL;
            return LV_RESULT_INVALID;
        }

        dsc->cache_entry = entry;
    }

    return res;
}

static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/

    if(dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
        lv_free((lv_draw_buf_t *)dsc->decoded);
    }
    else if(dsc->args.no_cache || !lv_image_cache_is_enabled()) {
        lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);
    }
}
