#ifndef _RES_FINE_TUNE_H_
#define _RES_FINE_TUNE_H_

typedef struct {
	int level;
	int up_flag; //1--upsample, 0--downsample
	int high_quality;  //1:high , 0-low
	int flag;    //0:bypass    1:trim
	int in_samp;	
	int out_samp;
	short *input_buf;
	short *output_buf;
}as_res_finetune_params;

typedef enum
{
    RES_FINETUNE_CMD_OPEN = 0,    
    RES_FINETUNE_CMD_CLOSE,        
    RES_FINETUNE_CMD_PROCESS,
} as_res_finetune_ops_cmd_t;

int as_res_finetune_ops(void *hnd, as_res_finetune_ops_cmd_t cmd, unsigned int args);

/*
void main()
{
	int len;
	int file_len;
	unsigned char *inbuf, *outbuf;
	unsigned char *inbuf_ptr, *outbuf_ptr;
	as_res_finetune_params param;
	int handle = 0;
	int rt;


	
	param.level = 0;
	rt = as_res_finetune_ops(&handle, RES_FINETUNE_CMD_OPEN, &param);


	param.high_quality = 1;
	param.in_samp = 8;
	param.flag = 1;
	while (1)
	{
		param.input_buf = inbuf;
		param.output_buf = outbuf;

		rt = as_res_finetune_ops(handle, RES_FINETUNE_CMD_PROCESS, &param);
		fwrite(outbuf, param.out_samp*sizeof(short), 1, fin);
		inbuf += param.in_samp*sizeof(short);
		
		if (param.out_samp != 8)
		{
			
			param.up_flag = !param.up_flag; //param.up_flag:1--Up sampling; 0-- Down sampling
		}
	}
	

	as_res_finetune_ops(handle, RES_FINETUNE_CMD_CLOSE, 0);
	return;
}*/

#endif
