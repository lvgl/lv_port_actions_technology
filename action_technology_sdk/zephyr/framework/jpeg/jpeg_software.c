#if LV_USE_SJPG
#include "lvgl.h"
#include <memory/mem_cache.h>
#include <spicache.h>
#include "src/extra/libs/sjpg/tjpgd.h"

#define TJPGD_WORKBUFF_SIZE             4096    //Recommended by TJPGD libray

typedef struct {
    uint8_t *sjpg_data;
    uint8_t *out_data;
    uint32_t data_size;
    uint32_t in_len;
    uint32_t out_size;
    int output_format;
    lv_area_t out_area;
} io_software_t;

static size_t input_func(JDEC * jd, uint8_t * buff, size_t ndata)
{
    io_software_t * io = jd->device;
    if(!io) return 0;
    uint32_t len = ndata;
    if(io->in_len + len > io->data_size)
        len = io->data_size - io->in_len;
    if(len == 0)
        return 0;
    if(buff)
        memcpy(buff, io->sjpg_data + io->in_len, len);
    io->in_len += len;
    return len;
}

static int img_data_cb(JDEC * jd, void * data, JRECT * rect)
{
    io_software_t *jpg_data = jd->device;
    lv_area_t in_rect = {
        .x1 = rect->left,
        .x2 = rect->right,
        .y1 = rect->top,
        .y2 = rect->bottom,
    };
    lv_area_t data_rect = {0};
    uint32_t cpy_len = 0;
    if (_lv_area_intersect(&data_rect, &jpg_data->out_area, &in_rect)) {
        lv_coord_t h = lv_area_get_height(&data_rect);
        lv_coord_t w = lv_area_get_width(&data_rect);
        uint32_t st_in = 0;
        if(data_rect.x1 > in_rect.x1)
            st_in = (data_rect.x1 - in_rect.x1) * 3;
        if (jpg_data->output_format) {  //rgb565
            uint8_t *in_data = (uint8_t *)data;
            uint16_t *out_data = (uint16_t *)jpg_data->out_data;
            uint32_t st_out = ((data_rect.y1 - jpg_data->out_area.y1) * lv_area_get_width(&jpg_data->out_area) + (data_rect.x1 - jpg_data->out_area.x1));
            for (lv_coord_t i = 0; i < h; i++) {
                for (lv_coord_t j = 0; j < w; j++) {
                    uint32_t in_idx = st_in + j * 3;
                    out_data[st_out + j] = ((in_data[in_idx] & 0xF8) << 8) | ((in_data[in_idx + 1] & 0xFC) << 3) | (in_data[in_idx + 2] >> 3);
                }
                st_in += lv_area_get_width(&in_rect) * 3;
                st_out += lv_area_get_width(&jpg_data->out_area);
                cpy_len += w * 2;
            } 
        } else {
            uint8_t *in_data = (uint8_t *)data;
            uint32_t st_out = ((data_rect.y1 - jpg_data->out_area.y1) * lv_area_get_width(&jpg_data->out_area) + (data_rect.x1 - jpg_data->out_area.x1)) * 3;
            for (lv_coord_t i = 0; i < h; i++) {
                lv_memcpy(jpg_data->out_data + st_out, in_data + st_in, w * 3);
                st_in += lv_area_get_width(&in_rect) * 3;
                st_out += lv_area_get_width(&jpg_data->out_area) * 3;
                cpy_len += w * 3;
            }
        }
        jpg_data->out_size += cpy_len;
    }
    return 1;
}

int jpg_software_decode(void* jpeg_src, int jpeg_size,
    void* bmp_buffer, int output_format, int output_stride,
    int win_x, int win_y, int win_w, int win_h)
{
    JDEC jd_tmp;
    uint8_t * workb_temp = lv_mem_alloc(TJPGD_WORKBUFF_SIZE);
    io_software_t jpg_data = {
        .out_data = (uint8_t *)bmp_buffer,
        .sjpg_data = jpeg_src,
        .data_size = jpeg_size,
        .output_format = output_format,
        .in_len = 0,
        .out_size = 0,
    };
    jpg_data.out_area.x1 = win_x;
    jpg_data.out_area.x2 = win_x + win_w - 1;
    jpg_data.out_area.y1 = win_y;
    jpg_data.out_area.y2 = win_y + win_h - 1;
    JRESULT rc = jd_prepare(&jd_tmp, input_func, workb_temp, (size_t)TJPGD_WORKBUFF_SIZE, &jpg_data);
    if(rc != JDR_OK) {
        LV_LOG_ERROR("jd_prepare error");
        lv_mem_free(workb_temp);
        return 0;
    }
    rc = jd_decomp(&jd_tmp, img_data_cb, 0);
    if (buf_is_psram(bmp_buffer)) {
        uint32_t out_len = jpg_data.output_format ? win_w * win_h * 2 : win_w * win_h * 3;
		mem_dcache_flush(bmp_buffer, out_len);
		mem_dcache_sync();
        mem_dcache_flush(bmp_buffer, out_len);
	}
    if(rc != JDR_OK)
        LV_LOG_ERROR("jd_decomp error");
    lv_mem_free(workb_temp);
    return jpg_data.out_size;
}

#endif
