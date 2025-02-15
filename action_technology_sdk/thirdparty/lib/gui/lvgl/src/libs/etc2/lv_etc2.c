/**
 * @file lv_etc2.c
 *
 */
/*********************
 *      INCLUDES
 *********************/
#include "../../draw/lv_image_decoder_private.h"
#include "../../../lvgl.h"

#if LV_USE_ETC2

#include "../../core/lv_global.h"

/*********************
 *      DEFINES
 *********************/
#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)

#define ETC2_RGB_NO_MIPMAPS             1
#define ETC2_RGBA_NO_MIPMAPS            3
#define ETC2_PKM_HEADER_SIZE            16
#define ETC2_PKM_FORMAT_OFFSET          6
#define ETC2_PKM_ENCODED_WIDTH_OFFSET   8
#define ETC2_PKM_ENCODED_HEIGHT_OFFSET  10
#define ETC2_PKM_WIDTH_OFFSET           12
#define ETC2_PKM_HEIGHT_OFFSET          14

#define ETC2_EAC_FORMAT_CODE            3

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if !LV_USE_DRAW_VG_LITE
extern void setupAlphaTable(void);
extern void decompressBlockAlphaC(uint8_t * data, uint8_t * img, int width, int height, int ix, int iy, int channels);
extern void decompressBlockETC2c(uint32_t block_part1, uint32_t block_part2, uint8_t * img, int width, int height, int startx, int starty, int channels);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

static uint16_t read_big_endian_uint16(const uint8_t * buf);

static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header);
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);

#if LV_USE_DRAW_VG_LITE
static lv_draw_buf_t * load_etc2_file(const char * filename, lv_image_decoder_dsc_t * dsc);
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_etc2_init(void)
{
    lv_image_decoder_t * decoder = lv_image_decoder_create();
    LV_ASSERT_MALLOC(decoder);
    if(decoder == NULL) {
        LV_LOG_WARN("lv_etc2_init: out of memory");
        return;
    }

    lv_image_decoder_set_info_cb(decoder, decoder_info);
    lv_image_decoder_set_open_cb(decoder, decoder_open);
    lv_image_decoder_set_close_cb(decoder, decoder_close);
    decoder->name = "ETC2";
}

void lv_etc2_deinit(void)
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

static uint16_t read_big_endian_uint16(const uint8_t * buf)
{
    return (buf[0] << 8) | buf[1];
}

#if !LV_USE_DRAW_VG_LITE
static uint32_t read_big_endian_uint32(const uint8_t * buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static void copy_block(uint8_t * dstbuf, uint32_t * pixel32, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t dst_pitch = w * 4;
    uint16_t copy_w = LV_MIN(w - x, 4);
    uint16_t copy_h = LV_MIN(h - y, 4);

    dstbuf += dst_pitch * y + x * 4;

    for(; copy_h > 0; copy_h--, pixel32 += 4, dstbuf += dst_pitch) {
        uint8_t *dst8 = dstbuf;
        uint8_t *pixel8 = (uint8_t *)pixel32;

        for(x = copy_w; x > 0; x--, pixel8 += 4, dst8 += 4) {
            dst8[0] = pixel8[2];
            dst8[1] = pixel8[1];
            dst8[2] = pixel8[0];
            dst8[3] = pixel8[3];
        }
    }
}

static lv_result_t decode_etc2(uint8_t * dstbuf, uint16_t w, uint16_t h, uint8_t * data, lv_fs_file_t * fp)
{
    uint8_t data8[16];
    uint32_t pixel32[16];

    setupAlphaTable();

    if(fp) data = data8;

    for(uint16_t y = 0; y < h; y += 4) {
        for(uint16_t x = 0; x < w; x += 4) {
            if(fp) {
                uint32_t rn;
                lv_fs_res_t res = lv_fs_read(fp, data, 16, &rn);
                if(res != LV_FS_RES_OK || rn != 16) {
                    LV_LOG_WARN("read ETC2 block failed");
                    return LV_FS_RES_UNKNOWN;
                }
            }

            uint8_t *pixel8 = (uint8_t *)pixel32;
            decompressBlockAlphaC(data, pixel8 + 3, 4, 4, 0, 0, 4);

            uint32_t block_part1 = read_big_endian_uint32(data + 8);
            uint32_t block_part2 = read_big_endian_uint32(data + 12);
            decompressBlockETC2c(block_part1, block_part2, pixel8, 4, 4, 0, 0, 4);

            copy_block(dstbuf, pixel32, x, y, w, h);

            if(fp == NULL) data += 16;
        }
    }

    return LV_FS_RES_OK;
}
#endif /* !LV_USE_DRAW_VG_LITE */

static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header)
{
    LV_UNUSED(decoder); /*Unused*/

    lv_image_src_t src_type = lv_image_src_get_type(dsc->src);

    if(src_type == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t *img_dsc = dsc->src;
        if(img_dsc->header.cf != LV_COLOR_FORMAT_ETC2_EAC) return LV_RESULT_INVALID;

        lv_memcpy(header, &img_dsc->header, sizeof(*header));
#if LV_USE_DRAW_VG_LITE == 0
        header->cf = LV_COLOR_FORMAT_ARGB8888;
        header->stride = header->w * 4;
#endif

        return LV_RESULT_OK;
    }
    else if(src_type == LV_IMAGE_SRC_FILE) {
        const char * ext = lv_fs_get_ext(dsc->src);
        if(!(lv_strcmp(ext, "pkm") == 0)) {
            return LV_RESULT_INVALID;
        }

        lv_fs_file_t f;
        lv_fs_res_t res = lv_fs_open(&f, dsc->src, LV_FS_MODE_RD);
        if(res != LV_FS_RES_OK) {
            LV_LOG_WARN("open %s failed", (const char *)dsc->src);
            return LV_RESULT_INVALID;
        }

        uint32_t rn;
        uint8_t pkm_header[ETC2_PKM_HEADER_SIZE];
        static const char pkm_magic[] = { 'P', 'K', 'M', ' ', '2', '0' };

        res = lv_fs_read(&f, pkm_header, ETC2_PKM_HEADER_SIZE, &rn);
        lv_fs_close(&f);

        if(res != LV_FS_RES_OK || rn != ETC2_PKM_HEADER_SIZE) {
            LV_LOG_WARN("Image get info read file magic number failed");
            return LV_RESULT_INVALID;
        }

        if(lv_memcmp(pkm_header, pkm_magic, sizeof(pkm_magic)) != 0) {
            LV_LOG_WARN("Image get info magic number invalid");
            return LV_RESULT_INVALID;
        }

        uint16_t pkm_format = read_big_endian_uint16(pkm_header + ETC2_PKM_FORMAT_OFFSET);
        if(pkm_format != ETC2_EAC_FORMAT_CODE) {
            LV_LOG_WARN("Image header format invalid : %d", pkm_format);
            return LV_RESULT_INVALID;
        }

        header->magic = LV_IMAGE_HEADER_MAGIC;
        header->cf = LV_COLOR_FORMAT_ETC2_EAC;
        header->w = read_big_endian_uint16(pkm_header + ETC2_PKM_WIDTH_OFFSET);
        header->h = read_big_endian_uint16(pkm_header + ETC2_PKM_HEIGHT_OFFSET);
        header->stride = read_big_endian_uint16(pkm_header + ETC2_PKM_ENCODED_WIDTH_OFFSET);
#if LV_USE_DRAW_VG_LITE == 0
        header->cf = LV_COLOR_FORMAT_ARGB8888;
        header->stride = header->w * 4;
#endif

        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID;
}

static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
#if LV_USE_DRAW_VG_LITE
    if(dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
        dsc->decoded = lv_malloc(sizeof(lv_draw_buf_t));
        if(dsc->decoded == NULL) return LV_RESULT_INVALID;

        lv_draw_buf_from_image((lv_draw_buf_t *)dsc->decoded, dsc->src);
        return LV_RESULT_OK;
    }
    else {
        dsc->decoded = load_etc2_file(dsc->src, dsc);
        if(dsc->decoded == NULL) return LV_RESULT_INVALID;

        if(!dsc->args.no_cache && lv_image_cache_is_enabled()) {
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

        return LV_RESULT_OK;
    }

#else
    lv_image_header_t * header = &dsc->header;
    lv_fs_res_t res = LV_FS_RES_OK;
    lv_draw_buf_t * decoded;

    decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers, header->w, header->h,
                                    LV_COLOR_FORMAT_ARGB8888, header->stride);
    if(decoded == NULL) {
        LV_LOG_ERROR("etc2 draw buff alloc %zu failed", (size_t)(header->stride * header->h));
        return LV_RESULT_INVALID;
    }

    if(dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
        res = decode_etc2(decoded->data, header->w, header->h, (uint8_t *)((lv_img_dsc_t *)dsc->src)->data, NULL);
    }
    else if(dsc->src_type == LV_IMAGE_SRC_FILE) {
        lv_fs_file_t f;
        res = lv_fs_open(&f, dsc->src, LV_FS_MODE_RD);
        if(res != LV_FS_RES_OK) {
            LV_LOG_WARN("etc2 decoder open %s failed", (const char *)dsc->src);
            lv_free(dsc->user_data);
            return LV_RESULT_INVALID;
        }

        res = lv_fs_seek(&f, ETC2_PKM_HEADER_SIZE, LV_FS_SEEK_SET);
        if(res != LV_FS_RES_OK) {
            LV_LOG_WARN("etc2 file seek %s failed", (const char *)dsc->src);
            lv_fs_close(&f);
            lv_free(dsc->user_data);
            return LV_RESULT_INVALID;
        }

        res = decode_etc2(decoded->data, header->w, header->h, NULL, &f);

        lv_fs_close(&f);
    }

    if(res != LV_FS_RES_OK) {
        lv_draw_buf_destroy(decoded);
        return LV_RESULT_INVALID;
    }

    dsc->decoded = decoded;

    if(!dsc->args.no_cache && lv_image_cache_is_enabled()) {
        lv_image_cache_data_t search_key;
        search_key.src_type = dsc->src_type;
        search_key.src = dsc->src;
        search_key.slot.size = decoded->data_size;

        lv_cache_entry_t * entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, NULL);
        if(entry == NULL) {
            lv_draw_buf_destroy(decoded);
            dsc->decoded = NULL;
            return LV_RESULT_INVALID;
        }

        dsc->cache_entry = entry;
    }

    return LV_RESULT_OK;
#endif /* LV_USE_DRAW_VG_LITE */
}

static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/

#if LV_USE_DRAW_VG_LITE
    if(dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
        lv_free((lv_draw_buf_t *)dsc->decoded);
    }
    else if(dsc->args.no_cache || !lv_image_cache_is_enabled()) {
        lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);
    }
#else
    if(dsc->args.no_cache || !lv_image_cache_is_enabled())
        lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);
#endif
}

#if LV_USE_DRAW_VG_LITE
static lv_draw_buf_t * load_etc2_file(const char * filename, lv_image_decoder_dsc_t * dsc)
{
    lv_image_header_t * header = &dsc->header;
    lv_draw_buf_t * decoded;

    decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers, header->w, header->h, header->cf, header->stride);
    if(decoded == NULL) {
        LV_LOG_ERROR("etc2 draw buff alloc %zu failed: %s", (size_t)(header->stride * header->h), filename);
        return NULL;
    }

    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, filename, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        LV_LOG_WARN("etc2 decoder open %s failed", (const char *)filename);
        goto failed;
    }

    res = lv_fs_seek(&f, ETC2_PKM_HEADER_SIZE, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        LV_LOG_WARN("etc2 file seek %s failed", (const char *)filename);
        lv_fs_close(&f);
        goto failed;
    }

    uint32_t rn;
    res = lv_fs_read(&f, decoded->data, decoded->data_size, &rn);
    lv_fs_close(&f);

    if(res != LV_FS_RES_OK || rn != decoded->data_size) {
        LV_LOG_WARN("etc2 read data failed, size:%zu", (size_t)decoded->data_size);
        goto failed;
    }

    return decoded;

failed:
    lv_draw_buf_destroy(decoded);
    return NULL;
}
#endif /* LV_USE_DRAW_VG_LITE */

#endif /*LV_USE_ETC2*/
