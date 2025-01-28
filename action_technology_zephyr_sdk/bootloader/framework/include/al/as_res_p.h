#ifndef __AS_RES_P_H__
#define __AS_RES_P_H__

/* argument for RES_CMD_OPEN and RES_CMD_MEM_REQUIRE */
typedef struct {
	/* input pcm samples parameters, both required in RES_CMD_OPEN and RES_CMD_MEM_REQUIRE */
	int channels;
	int samplerate_in;
	int samplerate_out;

	/* global memory allocated outside */
	void *global_buf_addr; /* must be 4 bytes aligned */
	int global_buf_len; /* retured by RES_CMD_MEM_REQUIRE */
	/* share memory allocated outside */
	void *share_buf_addr; /* must be 4 bytes aligned */
	int share_buf_len; /* retured by RES_CMD_MEM_REQUIRE */

	/* input/output samples per frame accepted in RES_CMD_PROCESS, returned by RES_CMD_OPEN */
	int input_samples;
	int output_samples;
} as_res_open_param_t;

/* argument of RES_CMD_PROCESS */
typedef struct {
	int channel; /* 0-left, 1-right */
	int input_samples;
	int output_samples;
	void *input_buf;
	void *output_buf;
} as_res_process_param_t;

typedef enum {
	RES_CMD_OPEN = 0,
	RES_CMD_CLOSE,
	RES_CMD_PROCESS,
	RES_CMD_MEM_REQUIRE,
} as_res_ops_cmd_t;

int as_res_ops(void *hnd, as_res_ops_cmd_t cmd, unsigned int args);

/*
main()
{
	as_res_open_param_t open_params;
	as_res_process_param_t process_params;
	void *handle = 0;
	int ret;

	//REQUIRE
	open_params.rate_in=16000;
	open_params.rate_out=44100;
	open_params.channels = 2;
	as_res_ops(handle, RES_CMD_MEM_REQUIRE, &open_params);

	//MALLOC,SYSTEM MANAGEMENT
	buf=(unsigned char*)malloc(open_params.require_mem_size);
	memset(buf,0,len);
	open_params.require_mem_ptr = buf;

	ret = as_res_ops(&handle,RES_CMD_OPEN,&params);
	if (ret || !handle) {
		printf("check Multiple instances ,max is 4 !\n");
		return;
	}

	//PROCESS
	while(1) {
		process_params.channel=0;//left channel
		process_params.input_buf = left_in_buf;
		process_params.output_buf = left_out_buf;
		process_params.input_samples = in_samples;
		as_res_ops(handle, RES_CMD_PROCESS, process_params);
		out_len = process_params.output_samples<<1;

		process_params.channel=1;//right channel
		process_params.input_buf = right_in_buf;
		process_params.output_buf = right_out_buf;
		as_res_ops(handle, RES_CMD_PROCESS, process_params);
		out_len = process_params.output_samples<<1;
	}

	as_res_ops(handle, RES_CMD_CLOSE, 0);
}
*/

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

////////////////////////////////////////////////////
// FIR HARDWARE
////////////////////////////////////////////////////
typedef struct {
	/* resample mode */
	int mode;
	/* history buffer (buf address must be 4 bytes aligned) */
	void *hist_buf;
	int hist_len;
	/* input pcm buffer (buf address must be 4 bytes aligned) */
	void *input_buf;
	int input_len;
	/* output pcm buffer (buf address must be 4 bytes aligned) */
	void *output_buf;
	int output_len; /* return the real processed output length */
} fir_param_t;

/**
 * @brief open fir device
 *
 * @return handle of fir device
 */
void *fir_device_open(void);

/**
 * @brief close fir device
 *
 * @param device handle of fir device
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
int fir_device_close(void *device);

/**
 * @brief run fir device
 *
 * @param device handle of fir device.
 * @param param  address of fir process parameter.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
int fir_device_run(void *device, fir_param_t *param);

/**
 * @endcond
 */

#endif /* __AS_RES_P_H__ */
