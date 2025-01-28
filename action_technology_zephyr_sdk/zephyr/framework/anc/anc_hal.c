
#include <assert.h>
#include <errno.h>
#include <devicetree.h>
#include <string.h>
#include <mem_manager.h>
#include "anc_inner.h"
#include "drivers/anc.h"
#include "os_common_api.h"
#include "drivers/audio/audio_in.h"
#include "drivers/audio/audio_out.h"
#include "soc_regs.h"
#include "anc_hal.h"

#define ANC_DSP_IMAGE "anc_dsp.dsp"
#define ANC_CFG_FF_FILE  "anccfgff.bin"
#define ANC_CFG_FB_FILE  "anccfgfb.bin"
#define ANC_CFG_TT_FILE  "anccfgtt.bin"

int anc_dsp_open(anc_mode_e mode)
{
	int ret = 0, i;
	char *cfg[2] = {NULL, NULL};
	uint32_t *anc_cfg = NULL;
	struct device *anc_dev = NULL;
	struct device *ain_dev = NULL;
	struct device *aout_dev = NULL;
	struct anc_imageinfo image;
	adc_anc_ctl_t adc_anc_ctl = {0};
	dac_anc_ctl_t dac_anc_ctl = {0};

	anc_dev = device_get_binding(CONFIG_ANC_NAME);
	ain_dev = device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
	aout_dev = device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if(!aout_dev || !ain_dev || !anc_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWEROFF)
	{
		SYS_LOG_ERR("anc already open\n");
		return -1;
	}

	memset(&image, 0, sizeof(image));

	if(sd_fmap(ANC_DSP_IMAGE, (void **)&image.ptr, (int *)&image.size))
	{
		SYS_LOG_ERR("cannot find anc image \"%s\"", ANC_DSP_IMAGE);
		return -1;
	}

	image.name = ANC_DSP_IMAGE;
	ret = anc_request_image(anc_dev, &image);
	if(ret)
	{
		SYS_LOG_ERR("anc request image err\n");
		return -1;
	}

	adc_anc_ctl.is_open_anc = 1;
	audio_in_control(ain_dev, NULL, AIN_CMD_ANC_CONTROL, &adc_anc_ctl);
	dac_anc_ctl.is_open_anc = 1;
	audio_out_control(aout_dev, NULL, AOUT_CMD_ANC_CONTROL, &dac_anc_ctl);

	ret = anc_poweron(anc_dev);
	if(ret)
	{
		SYS_LOG_ERR("anc power on err\n");
		goto err;
	}

	if(mode == ANC_MODE_ANC){
		cfg[0] = ANC_CFG_FF_FILE;
		cfg[1] = ANC_CFG_FB_FILE;
		SYS_LOG_INF("ANC MODE");
	}
	else{
		SYS_LOG_INF("TRANSPARENT MODE");
		cfg[0] = ANC_CFG_TT_FILE;
	}


	for(i=0; i<2; i++){
		if(cfg[i]){
			if(sd_fmap(cfg[i], (void **)&anc_cfg, NULL))
			{
				SYS_LOG_ERR("cannot find anc cfg file \"%s\"", cfg[i]);
				return -1;
			}

			if(anc_send_command(anc_dev, ANC_COMMAND_ANCTDATA, anc_cfg+1, *anc_cfg))
			{
				SYS_LOG_ERR("config anc err\n");
				goto err;
			}
		}
	}
	return 0;

err:
	anc_release_image(anc_dev);
	return -1;

}

int anc_dsp_close(void)
{
	struct device *anc_dev = NULL;
	struct device *ain_dev = NULL;
	struct device *aout_dev = NULL;
	adc_anc_ctl_t adc_anc_ctl = {0};
	dac_anc_ctl_t dac_anc_ctl = {0};

	anc_dev = device_get_binding(CONFIG_ANC_NAME);
	ain_dev = device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
	aout_dev = device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if(!aout_dev || !ain_dev || !anc_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) == ANC_STATUS_POWEROFF)
	{
		SYS_LOG_ERR("anc already close");
		return 0;
	}

	anc_send_command(anc_dev, ANC_COMMAND_POWEROFF, NULL, 0);

	/*wait dsp close*/
	os_sleep(5);

	anc_poweroff(anc_dev);

	anc_release_image(anc_dev);

	adc_anc_ctl.is_open_anc = 0;
	audio_in_control(ain_dev, NULL, AIN_CMD_ANC_CONTROL, &adc_anc_ctl);
	dac_anc_ctl.is_open_anc = 0;
	audio_out_control(aout_dev, NULL, AOUT_CMD_ANC_CONTROL, &dac_anc_ctl);

	return 0;
}

int anc_dsp_send_anct_data(void *data, int size)
{
	struct device *anc_dev = NULL;

	anc_dev = device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL)
	{
		SYS_LOG_ERR("get anc device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON)
	{
		SYS_LOG_ERR("anc not open");
		return 0;
	}

	return anc_send_command(anc_dev, ANC_COMMAND_ANCTDATA, data, size);
}


int anc_dsp_dump_data(int start, uint32_t ringbuf_addr)
{
	struct device *anc_dev = NULL;

	anc_dev = device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL)
	{
		SYS_LOG_ERR("get anc device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON)
	{
		SYS_LOG_ERR("anc not open");
		return 0;
	}

	if(start){
		return anc_send_command(anc_dev, ANC_COMMAND_DUMPSTART, &ringbuf_addr, 4);
	}
	else{
		return anc_send_command(anc_dev, ANC_COMMAND_DUMPSTOP, NULL, 0);
	}
}

int anc_dsp_samplerate_notify(void)
{
	return 0;
}


