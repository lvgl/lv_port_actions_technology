

#ifndef _ANC_HAL_H_
#define _ANC_HAL_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	ANC_MODE_ANC,
	ANC_MODE_TRANS,
}anc_mode_e;

/**
 * @brief open and load image to anc dsp
 *
 * @return 0 if success open,otherwise return none zero
 */
int anc_dsp_open(anc_mode_e mode);

/**
 * @brief close anc dsp
 *
 * @return 0 if success open,otherwise return none zero
 */
int anc_dsp_close(void);

/**
 * @brief send command to anc dsp
 *
 * @param data Address of anct data
 * @param data Size of anct data
 *
 * @return 0 if send command success,none zero if failed
 * @note the @size must be 2 bytes aligned ,anct tool data length is 364 bytes
 */
int anc_dsp_send_anct_data(void *data, int size);

/**
 * @brief get one frame pcm data frome anc dsp
 *
 * @param start 1:start dump data; 0:stop dump data
 * @param ringbuf_addr address of ringbuf that dsp write data to
 * @return 0 if success,-1 if failed
 */
int anc_dsp_dump_data(int start, uint32_t ringbuf_addr);

/**
 * @brief notify anc dsp that samplerate has change
 *
 * @return 0 if success,-1 if failed
 */
int anc_dsp_samplerate_notify(void);
#ifdef __cplusplus
}
#endif

#endif
