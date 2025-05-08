#ifndef JPEG_SOFTWARE
#define JPEG_SOFTWARE

#ifdef __cplusplus
extern "C" {
#endif

#if LV_USE_SJPG

int jpg_software_decode(void* jpeg_src, int jpeg_size,
    void* bmp_buffer, int output_format, int output_stride,
    int win_x, int win_y, int win_w, int win_h);

#endif

#ifdef __cplusplus
}
#endif

#endif
