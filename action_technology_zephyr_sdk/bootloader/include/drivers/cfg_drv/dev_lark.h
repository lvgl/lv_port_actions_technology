/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEV_LARK_H_
#define DEV_LARK_H_

/*
dma cfg
*/
#define  CONFIG_DMA_0_NAME                  "DMA_0"
#define  CONFIG_DMA_0_PCHAN_NUM              10
#define  CONFIG_DMA_0_VCHAN_NUM              10
#define  CONFIG_DMA_0_VCHAN_PCHAN_NUM        6
#define  CONFIG_DMA_0_VCHAN_PCHAN_START      4


/*
DSP cfg
*/
#define  CONFIG_DSP_NAME                  "DSP"

/*
ANC cfg
*/
#define CONFIG_ANC_NAME			  "anc"

/*
BTC cfg
*/
#define  CONFIG_BTC_NAME                  "BTC"

/*
 * Audio cfg
 */
#ifndef CONFIG_AUDIO_OUT_ACTS_DEV_NAME
#define CONFIG_AUDIO_OUT_ACTS_DEV_NAME  "audio_out"
#endif
#ifndef CONFIG_AUDIO_IN_ACTS_DEV_NAME
#define CONFIG_AUDIO_IN_ACTS_DEV_NAME   "audio_in"
#endif

/*
 * DSP cfg
 */
#ifndef CONFIG_DSP_ACTS_DEV_NAME
#define CONFIG_DSP_ACTS_DEV_NAME  "dsp_acts"
#endif


/*
PMUADC cfg
*/
#define	PMUADC_ID_CHARGI		(0)
#define	PMUADC_ID_BATV			(1)
#define	PMUADC_ID_DC5V			(2)
#define PMUADC_ID_SENSOR		(3)
#define PMUADC_ID_SVCC			(4)
#define	PMUADC_ID_LRADC1		(5)
#define	PMUADC_ID_LRADC2		(6)
#define	PMUADC_ID_LRADC3		(7)
#define	PMUADC_ID_LRADC4		(8)
#define	PMUADC_ID_LRADC5		(9)
#define	PMUADC_ID_LRADC6		(10)
#define	PMUADC_ID_LRADC7		(11)

#endif /* DEV_LARK_H_ */
