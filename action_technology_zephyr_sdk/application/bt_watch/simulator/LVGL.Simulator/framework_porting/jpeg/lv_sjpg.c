/**
 * @file lv_sjpg.c
 *
 */

/*----------------------------------------------------------------------------------------------------------------------------------
/    Added normal JPG support [7/10/2020]
/    ----------
/    SJPEG is a custom created modified JPEG file format for small embedded platforms.
/    It will contain multiple JPEG fragments all embedded into a single file with a custom header.
/    This makes JPEG decoding easier using any JPEG library. Overall file size will be almost
/    similar to the parent jpeg file. We can generate sjpeg from any jpeg using a python script
/    provided along with this project.
/                                                                                     (by vinodstanur | 2020 )
/    SJPEG FILE STRUCTURE
/    --------------------------------------------------------------------------------------------------------------------------------
/    Bytes                       |   Value                                                                                           |
/    --------------------------------------------------------------------------------------------------------------------------------
/
/    0 - 7                       |   "_SJPG__" followed by '\0'
/
/    8 - 13                      |   "V1.00" followed by '\0'       [VERSION OF SJPG FILE for future compatibiliby]
/
/    14 - 15                     |   X_RESOLUTION (width)            [little endian]
/
/    16 - 17                     |   Y_RESOLUTION (height)           [little endian]
/
/    18 - 19                     |   TOTAL_FRAMES inside sjpeg       [little endian]
/
/    20 - 21                     |   JPEG BLOCK WIDTH (16 normally)  [little endian]
/
/    22 - [(TOTAL_FRAMES*2 )]    |   SIZE OF EACH JPEG SPLIT FRAGMENTS   (FRAME_INFO_ARRAY)
/
/   SJPEG data                   |   Each JPEG frame can be extracted from SJPEG data by parsing the FRAME_INFO_ARRAY one time.
/
/----------------------------------------------------------------------------------------------------------------------------------
/                   JPEG DECODER
/                   ------------
/   We are using TJpgDec - Tiny JPEG Decompressor library from ELM-CHAN for decoding each split-jpeg fragments.
/   The tjpgd.c and tjpgd.h is not modified and those are used as it is. So if any update comes for the tiny-jpeg,
/   just replace those files with updated files.
/---------------------------------------------------------------------------------------------------------------------------------*/

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include "lv_sjpg.h"

/*********************
 *      DEFINES
 *********************/
#define TJPGD_WORKBUFF_SIZE             4096    //Recommended by TJPGD libray

//NEVER EDIT THESE OFFSET VALUES
#define SJPEG_VERSION_OFFSET            8
#define SJPEG_X_RES_OFFSET              14
#define SJPEG_y_RES_OFFSET              16
#define SJPEG_TOTAL_FRAMES_OFFSET       18
#define SJPEG_BLOCK_WIDTH_OFFSET        20
#define SJPEG_FRAME_INFO_ARRAY_OFFSET   22

/**********************
 *      TYPEDEFS
 **********************/

enum io_source_type {
    SJPEG_IO_SOURCE_C_ARRAY,
    SJPEG_IO_SOURCE_DISK,
};

typedef struct {
    enum io_source_type type;
    lv_fs_file_t lv_file;
    uint8_t * img_cache_buff;
    int img_cache_x_res;
    int img_cache_y_res;
    uint8_t * raw_sjpg_data;              //Used when type==SJPEG_IO_SOURCE_C_ARRAY.
    uint32_t raw_sjpg_data_size;          //Num bytes pointed to by raw_sjpg_data.
    uint32_t raw_sjpg_data_next_read_pos; //Used for all types.
} io_source_t;


typedef struct {
    uint8_t * sjpeg_data;
    uint32_t sjpeg_data_size;
    int sjpeg_x_res;
    int sjpeg_y_res;
    int sjpeg_total_frames;
    int sjpeg_single_frame_height;
    int sjpeg_cache_frame_index;
    uint8_t ** frame_base_array;        //to save base address of each split frames upto sjpeg_total_frames.
    int * frame_base_offset;            //to save base offset for fseek
    uint8_t * frame_cache;
    uint8_t * workb;                    //JPG work buffer for jpeg library
    JDEC * tjpeg_jd;
    io_source_t io;
} SJPEG;

/**********************
 *  STATIC PROTOTYPES
 **********************/


static size_t input_func(JDEC * jd, uint8_t * buff, size_t ndata);
static int output_func(JDEC* jd, void* data, JRECT* rect);



void lv_split_jpeg_init(void)
{
}

static int output_func(JDEC * jd, void * data, JRECT * rect)
{
    io_source_t * io = jd->device;
    uint8_t * cache = io->img_cache_buff;
    const int xres = io->img_cache_x_res;
    uint8_t * buf = data;
    const int INPUT_PIXEL_SIZE = 3;
    const int row_width = rect->right - rect->left + 1; // Row width in pixels.
    const int row_size = row_width * INPUT_PIXEL_SIZE;  // Row size (bytes).

    for(int y = rect->top; y <= rect->bottom; y++) {
        int row_offset = y * xres * INPUT_PIXEL_SIZE + rect->left * INPUT_PIXEL_SIZE;
        memcpy(cache + row_offset, buf, row_size);
        buf += row_size;
    }

    return 1;
}

static size_t input_func(JDEC * jd, uint8_t * buff, size_t ndata)
{
    io_source_t * io = jd->device;

    if(!io) return 0;

    if(io->type == SJPEG_IO_SOURCE_C_ARRAY) {
        const uint32_t bytes_left = io->raw_sjpg_data_size - io->raw_sjpg_data_next_read_pos;
        const uint32_t to_read = ndata <= bytes_left ? (uint32_t)ndata : bytes_left;
        if(to_read == 0)
            return 0;
        if(buff) {
            memcpy(buff, io->raw_sjpg_data + io->raw_sjpg_data_next_read_pos, to_read);
        }
        io->raw_sjpg_data_next_read_pos += to_read;
        return to_read;
    }
    return 0;
}

lv_res_t decoder_jpeg(uint8_t* buf, uint32_t buf_size, uint8_t* buf_out, uint16_t out_stride,
                      uint32_t x, uint32_t y, uint32_t win_w, uint32_t win_h)
{
    io_source_t io;
    io.type = SJPEG_IO_SOURCE_C_ARRAY;
    io.raw_sjpg_data = buf;
    io.raw_sjpg_data_size = buf_size;
    io.raw_sjpg_data_next_read_pos = 0;

    JDEC jdec;
    void* work = malloc(3500);

    JRESULT res;

    res = jd_prepare(&jdec, input_func, work, 3500, &io);
    if (res != JDR_OK)
    {
        free(work);
        return res;
    }

    int w = jdec.width;
    int h = jdec.height;

    io.img_cache_buff = malloc(sizeof(uint8_t) * w * h * 3);
    io.img_cache_x_res = w;
    io.img_cache_y_res = h;

    res = jd_decomp(&jdec, output_func, 0);
    if (res != JDR_OK)
    {
        free(work);
        free(io.img_cache_buff);
        return res;
    }

    int offset_temp = 0;
    int ptr_offset = 0;

#if LV_COLOR_DEPTH == 32
    for (int __y = 0; __y < h; __y++)
    {
        for (int __x = 0; __x < w; __x++)
        {
            int index = __y * w + __x;

            uint8_t val1 = 0xff;
            uint8_t val2 = *io.img_cache_buff++;
            uint8_t val3 = *io.img_cache_buff++;
            uint8_t val4 = *io.img_cache_buff++;
            ptr_offset += 3;

            if (__x < x || __y < y)
                continue;

            if (__x > x + win_w - 1 || __y > y + win_h - 1)
                continue;

            buf_out[offset_temp + 3] = val1;
            buf_out[offset_temp + 2] = val2;
            buf_out[offset_temp + 1] = val3;
            buf_out[offset_temp + 0] = val4;
            offset_temp += 4;
        }
    }

#elif  LV_COLOR_DEPTH == 16
    uint8_t* temp_img_cache_buff = io.img_cache_buff;
    for (int __y = 0; __y < h; __y++)
    {
        offset_temp = out_stride * (__y - y) * 2;
        for (int __x = 0; __x < w; __x++)
        {
            int index = __y * w + __x;

            uint16_t col_16bit = (*temp_img_cache_buff++ & 0xf8) << 8;
            col_16bit |= (*temp_img_cache_buff++ & 0xFC) << 3;
            col_16bit |= (*temp_img_cache_buff++ >> 3);
            ptr_offset += 3;

            if (__x < x || __y < y)
                continue;

            if (__x > x + win_w - 1 || __y > y + win_h - 1)
                continue;

#if  LV_BIG_ENDIAN_SYSTEM == 1 || LV_COLOR_16_SWAP == 1
            buf_out[offset_temp++] = col_16bit >> 8;
            buf_out[offset_temp++] = col_16bit & 0xff;

#else
            buf_out[offset_temp++] = col_16bit & 0xff;
            buf_out[offset_temp++] = col_16bit >> 8;
#endif // LV_BIG_ENDIAN_SYSTEM
        }
    }

#elif  LV_COLOR_DEPTH == 8

    for (int __y = 0; __y < h; __y++)
    {
        for (int __x = 0; __x < w; __x++)
        {
            int index = __y * w + __x;

            uint8_t col_8bit = (*io.img_cache_buff++ & 0xC0)
            col_8bit |= (*io.img_cache_buff++ & 0xe0) >> 2;
            col_8bit |= (*io.img_cache_buff++ & 0xe0) >> 5;
            ptr_offset += 3;

            if (__x < x || __y < y)
                continue;

            if (__x > x + win_w - 1 || __y > y + win_h - 1)
                continue;

            buf_out[offset_temp++] = col_8bit;
        }
    }

#else
#error Unsupported LV_COLOR_DEPTH
#endif // LV_COLOR_DEPTH
    
    free(work);
    free(io.img_cache_buff);

    return res;
}
