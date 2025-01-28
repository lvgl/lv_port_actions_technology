/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#include <lvgl.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    RLE_STATE_SINGLE = 0,
    RLE_STATE_REPEATED,
    RLE_STATE_COUNTER,
} font_fmt_rle_state_t;

typedef struct {
    uint32_t rdp;
    const uint8_t * in;
    uint8_t bpp;
    uint8_t prev_v;
    uint8_t count;
    font_fmt_rle_state_t state;
} font_fmt_rle_t;

static const uint8_t opa4_table[16] = {0,  17, 34,  51,
                                       68, 85, 102, 119,
                                       136, 153, 170, 187,
                                       204, 221, 238, 255
                                      };

static const uint8_t opa3_table[8] = {0, 36, 73, 109, 146, 182, 218, 255};

static const uint8_t opa2_table[4] = {0, 85, 170, 255};

static font_fmt_rle_t bmp_font_fmt_rle;


/**********************
 *  STATIC PROTOTYPES
 **********************/
void decompress_glyf_bitmap(const uint8_t * in, uint8_t * out, int16_t w, int16_t h, uint8_t bpp, bool prefilter, uint8_t* linebuf1, uint8_t* linebuf2);
static inline void decompress_line(uint8_t * out, int16_t w);
static inline uint8_t get_bits(const uint8_t * in, uint32_t bit_pos, uint8_t len);
//static inline void bits_write(uint8_t * out, uint32_t bit_pos, uint8_t val, uint8_t len);
static inline void rle_init(const uint8_t * in,  uint8_t bpp);
static inline uint8_t rle_next(void);



/**********************
 *  STATIC VARIABLES
 **********************/
/*
static uint32_t rle_rdp;
static const uint8_t * rle_in;
static uint8_t rle_bpp;
static uint8_t rle_prev_v;
static uint8_t rle_cnt;
static rle_state_t rle_state;
*/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * The compress a glyph's bitmap
 * @param in the compressed bitmap
 * @param out buffer to store the result
 * @param px_num number of pixels in the glyph (width * height)
 * @param bpp bit per pixel (bpp = 3 will be converted to bpp = 4)
 * @param prefilter true: the lines are XORed
 */
void decompress_glyf_bitmap(const uint8_t * in, uint8_t * out, int16_t w, int16_t h, uint8_t bpp, bool prefilter, uint8_t* linebuf1, uint8_t* linebuf2)
{
    const lv_opa_t * opa_table;
    uint8_t out_x;
    //uint8_t ppb = 8/bpp;
    switch(bpp) {
        case 2:
            opa_table = opa2_table;
            break;
        case 3:
            opa_table = opa3_table;
            break;
        case 4:
            opa_table = opa4_table;
            break;
        default:
            LV_LOG_WARN("%d bpp is not handled", bpp);
            return;
    }

    rle_init(in, bpp);

    uint8_t* line_buf1 = linebuf1;

    uint8_t * line_buf2 = NULL;

    if(prefilter) {		
        line_buf2 = linebuf2;
    }

    decompress_line(line_buf1, w);

    int32_t y;
    int32_t x;
    //uint32_t stride = lv_draw_buf_width_to_stride(w, LV_COLOR_FORMAT_A8);
    uint32_t stride = (w * bpp + 7)/8;

    for(x = 0; x < w; x++) {
        //out[x] = opa_table[line_buf1[x]];
        out_x = opa_table[line_buf1[x]];
        if (bpp == 2) {
            out[x/4] |= (out_x&0xc0)>>((x%4)*2);
        } else if (bpp == 4) {
            out[x/2] |= (out_x&0xf0)>>((x%2)*2);
        } else {

        }
    }
    out += stride;

    for(y = 1; y < h; y++) {
        if(prefilter) {
            decompress_line(line_buf2, w);

            for(x = 0; x < w; x++) {
                line_buf1[x] = line_buf2[x] ^ line_buf1[x];
                //out[x] = opa_table[line_buf1[x]];
                out_x = opa_table[line_buf1[x]];
                if (bpp == 2) {
                    out[x/4] |= (out_x&0xc0)>>((x%4)*2);                    
                } else if (bpp == 4) {
                    out[x/2] |= (out_x&0xf0)>>((x%2)*2);                   
                } else {

                }                
            }
        }
        else {
            decompress_line(line_buf1, w);

            for(x = 0; x < w; x++) {
                //out[x] = opa_table[line_buf1[x]];
                out_x = opa_table[line_buf1[x]];
                if (bpp == 2) {
                    out[x/4] |= (out_x&0xc0)>>((x%4)*2);                    
                } else if (bpp == 4) {
                    out[x/2] |= (out_x&0xf0)>>((x%2)*2);                   
                } else {

                }                 
            }
        }
        out += stride;
    }
}

/**
 * Decompress one line. Store one pixel per byte
 * @param out output buffer
 * @param w width of the line in pixel count
 */
static inline void decompress_line(uint8_t * out, int16_t w)
{
    int16_t i;
    for(i = 0; i < w; i++) {
        out[i] = rle_next();
    }
}

/**
 * Read bits from an input buffer. The read can cross byte boundary.
 * @param in the input buffer to read from.
 * @param bit_pos index of the first bit to read.
 * @param len number of bits to read (must be <= 8).
 * @return the read bits
 */
static inline uint8_t get_bits(const uint8_t * in, uint32_t bit_pos, uint8_t len)
{
    uint8_t bit_mask;
    switch(len) {
        case 1:
            bit_mask = 0x1;
            break;
        case 2:
            bit_mask = 0x3;
            break;
        case 3:
            bit_mask = 0x7;
            break;
        case 4:
            bit_mask = 0xF;
            break;
        case 8:
            bit_mask = 0xFF;
            break;
        default:
            bit_mask = (uint16_t)((uint16_t) 1 << len) - 1;
    }

    uint32_t byte_pos = bit_pos >> 3;
    bit_pos = bit_pos & 0x7;

    if(bit_pos + len >= 8) {
        uint16_t in16 = (in[byte_pos] << 8) + in[byte_pos + 1];
        return (in16 >> (16 - bit_pos - len)) & bit_mask;
    }
    else {
        return (in[byte_pos] >> (8 - bit_pos - len)) & bit_mask;
    }
}

static inline void rle_init(const uint8_t * in,  uint8_t bpp)
{
    font_fmt_rle_t * rle = &bmp_font_fmt_rle;
    rle->in = in;
    rle->bpp = bpp;
    rle->state = RLE_STATE_SINGLE;
    rle->rdp = 0;
    rle->prev_v = 0;
    rle->count = 0;
}

static inline uint8_t rle_next(void)
{
    uint8_t v = 0;
    uint8_t ret = 0;
    font_fmt_rle_t * rle = &bmp_font_fmt_rle;

    if(rle->state == RLE_STATE_SINGLE) {
        ret = get_bits(rle->in, rle->rdp, rle->bpp);
        if(rle->rdp != 0 && rle->prev_v == ret) {
            rle->count = 0;
            rle->state = RLE_STATE_REPEATED;
        }

        rle->prev_v = ret;
        rle->rdp += rle->bpp;
    }
    else if(rle->state == RLE_STATE_REPEATED) {
        v = get_bits(rle->in, rle->rdp, 1);
        rle->count++;
        rle->rdp += 1;
        if(v == 1) {
            ret = rle->prev_v;
            if(rle->count == 11) {
                rle->count = get_bits(rle->in, rle->rdp, 6);
                rle->rdp += 6;
                if(rle->count != 0) {
                    rle->state = RLE_STATE_COUNTER;
                }
                else {
                    ret = get_bits(rle->in, rle->rdp, rle->bpp);
                    rle->prev_v = ret;
                    rle->rdp += rle->bpp;
                    rle->state = RLE_STATE_SINGLE;
                }
            }
        }
        else {
            ret = get_bits(rle->in, rle->rdp, rle->bpp);
            rle->prev_v = ret;
            rle->rdp += rle->bpp;
            rle->state = RLE_STATE_SINGLE;
        }

    }
    else if(rle->state == RLE_STATE_COUNTER) {
        ret = rle->prev_v;
        rle->count--;
        if(rle->count == 0) {
            ret = get_bits(rle->in, rle->rdp, rle->bpp);
            rle->prev_v = ret;
            rle->rdp += rle->bpp;
            rle->state = RLE_STATE_SINGLE;
        }
    }

    return ret;
}
