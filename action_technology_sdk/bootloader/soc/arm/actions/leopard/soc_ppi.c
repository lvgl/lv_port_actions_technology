/*******************************************************************************
 * @file    soc_ppi.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   lark hardware access layer
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <soc.h>
#include <linker/linker-defs.h>

/******************************************************************************/
//typedefs
/******************************************************************************/
typedef struct {                                
  volatile uint32_t  TRIGGER_EN;       /*!< (@ 0x00000000) Trigger Enable Register */
  volatile uint32_t  TRIGGER_EN1;      /*!< (@ 0x00000004) Trigger Enable Register */
  volatile uint32_t  TRIGGER_PD;       /*!< (@ 0x00000008) Trigger Pending Register */
  volatile uint32_t  TRIGGER_PD1;      /*!< (@ 0x0000000C) Trigger Pending Register */
  volatile uint32_t  PPI_CH_CFG[12];   /*!< (@ 0x00000010) PPI Channel Config Register */
} WIC_Type;

#define WIC                               ((WIC_Type*) WIC_BASE)

/* PPI_CH_CFG */
#define WIC_PPI_CH_CFG_CH_EN_Pos          (31UL)            /*!< CH_EN (Bit 31) */
#define WIC_PPI_CH_CFG_CH_EN_Msk          (0x80000000UL)    /*!< CH_EN (Bitfield-Mask: 0x01) */
#define WIC_PPI_CH_CFG_TASK_SEL_Pos       (8UL)             /*!< TASK_SEL (Bit 8) */
#define WIC_PPI_CH_CFG_TASK_SEL_Msk       (0x1f00UL)        /*!< TASK_SEL (Bitfield-Mask: 0x1f) */
#define WIC_PPI_CH_CFG_TRIGGER_SEL_Pos    (0UL)             /*!< TRIGGER_SEL (Bit 0) */
#define WIC_PPI_CH_CFG_TRIGGER_SEL_Msk    (0x3fUL)          /*!< TRIGGER_SEL (Bitfield-Mask: 0x3f) */

/******************************************************************************/
//functions
/******************************************************************************/
static __sleepfunc int ppi_trig_src_mapping(int trig_src)
{
	if (trig_src <= TIMER4) {
		return trig_src;
	} else if (trig_src <= IO11_IRQ) {
		return (trig_src - IO0_IRQ + 16);
	} else if (trig_src <= SPIMT1_TASK7_CIP) {
		return (trig_src - SPIMT0_TASK0_CIP + 32);
	} else if (trig_src <= I2CMT0_TASK3_CIP) {
		return (trig_src - I2CMT0_TASK0_CIP + 48);
	} else {
		return (trig_src - I2CMT1_TASK0_CIP + 56);
	}
}

void ppi_init(void)
{
	int ch;
	
	/* clear trigger enable */
	WIC->TRIGGER_EN = 0;
	WIC->TRIGGER_EN1 = 0;
	/* clear trigger pending */
	WIC->TRIGGER_PD = WIC->TRIGGER_PD;
	WIC->TRIGGER_PD1 = WIC->TRIGGER_PD1;
	/* disable channel */
	for (ch = PPI_CH0; ch <= PPI_CH11; ch ++) {
		WIC->PPI_CH_CFG[ch] &= ~WIC_PPI_CH_CFG_CH_EN_Msk;
	}
}

void ppi_trig_src_en(int trig_src, int enable)
{
	int bit_offset = ppi_trig_src_mapping(trig_src);
	
	/* enable trigger source */
	if (bit_offset < 32) {
		if (enable) {
			WIC->TRIGGER_EN |= (1 << bit_offset);
		} else {
			WIC->TRIGGER_EN &= ~(1 << bit_offset);
		}
	}	else {
		if (enable) {
			WIC->TRIGGER_EN1 |= (1 << (bit_offset - 32));
		} else {
			WIC->TRIGGER_EN1 &= ~(1 << (bit_offset - 32));
		}
	}
}

void ppi_task_trig_config(int ppi_channel, int task_select, int trig_src_select)
{
	/* disable channel */
	WIC->PPI_CH_CFG[ppi_channel] &= ~WIC_PPI_CH_CFG_CH_EN_Msk;
	WIC->PPI_CH_CFG[ppi_channel] |= ((unsigned int)0 << WIC_PPI_CH_CFG_CH_EN_Pos);
	
	/* select task */
	WIC->PPI_CH_CFG[ppi_channel] &= ~WIC_PPI_CH_CFG_TASK_SEL_Msk;
	WIC->PPI_CH_CFG[ppi_channel] |= (task_select << WIC_PPI_CH_CFG_TASK_SEL_Pos);
	
	/* select trigger source */
	WIC->PPI_CH_CFG[ppi_channel] &= ~WIC_PPI_CH_CFG_TRIGGER_SEL_Msk;
	WIC->PPI_CH_CFG[ppi_channel] |= (trig_src_select << WIC_PPI_CH_CFG_TRIGGER_SEL_Pos);
	
	/* enable channel */
	WIC->PPI_CH_CFG[ppi_channel] &= ~WIC_PPI_CH_CFG_CH_EN_Msk;
	WIC->PPI_CH_CFG[ppi_channel] |= ((unsigned int)1 << WIC_PPI_CH_CFG_CH_EN_Pos);
}

int ppi_trig_src_is_pending(int ppi_trig_src)
{
	int bit_offset = ppi_trig_src_mapping(ppi_trig_src);
	int pending;
	
	if (bit_offset < 32) {
		pending = (WIC->TRIGGER_PD & (1 << bit_offset));
	}	else {
		pending = (WIC->TRIGGER_PD1 & (1 << (bit_offset - 32)));
	}
	
	return pending;
}

__sleepfunc void ppi_trig_src_clr_pending(int ppi_trig_src)
{
	int bit_offset = ppi_trig_src_mapping(ppi_trig_src);
	
	if (bit_offset < 32) {
		WIC->TRIGGER_PD = (1 << bit_offset);
	}	else {
		WIC->TRIGGER_PD1 = (1 << (bit_offset - 32));
	}
	
	return;
}
