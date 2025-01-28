#include <gcc_include.h>
#include <soc.h>
#include <att_debug.h>

#define	HOSC_CTL_HOSC_CAP_MASK			(0xFF<<17)
#define	HOSC_CTL_HOSC_CAP_SHIFT			17
#define	HOSC_CTL_HOSC_CAP(x)	 		((x) << HOSC_CTL_HOSC_CAP_SHIFT)

#define	HOSCLDO_CTL_OSC32M_EN					6
#define	HOSCLDO_CTL_OSC32M_CALMODE      		8
#define	HOSCLDO_CTL_OSC32M_CALEN        		7
#define	HOSCLDO_CTL_OSC32M_FRQMSET_SHIFT		11
#define	HOSCLDO_CTL_OSC32M_CALDONE				21
#define	HOSCLDO_CTL_OSC32M_FRQCAL_SHIFT     	22
#define	HOSCLDO_CTL_OSC32M_FRQCAL_MASK			(0x3FF<<22)

/*!
 * \brief get the HOSC capacitance
 * \n  capacitance is x10 fixed value
 */
extern int soc_get_hosc_cap(void)
{
    int  cap = 0;

    uint32_t hosc_ctl = sys_read32(HOSC_CTL);

    cap = (hosc_ctl >> 17) & 0xff;

    return cap;
}

static void ipmsg_btc_soc_set_hoscldo_ctl(void)
{
	int val, i, j, cnt;

	sys_write32(0x80100204, HOSCLDO_CTL);
	mdelay(10);
	val = sys_read32(HOSCLDO_CTL) | (0x1 << HOSCLDO_CTL_OSC32M_EN);
	sys_write32(val, HOSCLDO_CTL);
	mdelay(10);
	val = sys_read32(HOSCLDO_CTL) | (0x1 << HOSCLDO_CTL_OSC32M_CALEN) | (0x1 << HOSCLDO_CTL_OSC32M_CALMODE);
	sys_write32(val, HOSCLDO_CTL);

	i = 0;
	while ((sys_read32(HOSCLDO_CTL) & (0x1 << HOSCLDO_CTL_OSC32M_CALDONE))) {
		if (i++ > 10) {
			LOG_I("OSC32M 0 timeout\n");
			break;
		}

		mdelay(1);
	}

	i = 0;
	while (1) {
		cnt = 0;
		for (j=0; j<3; j++) {
			if (sys_read32(HOSCLDO_CTL) & (0x1 << HOSCLDO_CTL_OSC32M_CALDONE)) {
				cnt++;
			}
			mdelay(10);
		}

		if (cnt == 3) {
			break;
		}

		if (i++ > 50) {
			LOG_I("OSC32M 1 timeout\n");
			break;
		}
		mdelay(1);
	}

	val = sys_read32(HOSCLDO_CTL) & HOSCLDO_CTL_OSC32M_FRQCAL_MASK;
	val >>= HOSCLDO_CTL_OSC32M_FRQCAL_SHIFT;
	val <<= HOSCLDO_CTL_OSC32M_FRQMSET_SHIFT;
	val |= 0x80000204;
	sys_write32(val, HOSCLDO_CTL);
}

/*!
 * \brief set the HOSC capacitance
 * \n  capacitance is x10 fixed value
 */
extern void soc_set_hosc_cap(int cap)
{
	int val;

	LOG_I("Set hosc cap %d.%d pf\n", cap / 10, cap % 10);

	val = sys_read32(HOSC_CTL);
	val &= (~HOSC_CTL_HOSC_CAP_MASK);
	val |= HOSC_CTL_HOSC_CAP(cap);
	sys_write32(val, HOSC_CTL);

	ipmsg_btc_soc_set_hoscldo_ctl();
	LOG_I("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));
}

#define     HOSC_CTL_HOSC_READY                                               28
static void leopard_set_hosc_ctrl()
{
	/*
	IO_WRITE(HOSC_CTL, (0x601f1b6 | (CAP<<17)));
	如果是首次配置（HOSC_CTL[25]: 0->1）需要将CPUCLK切换到RC4M，等待HOSC_CTL[28]为1后再将CPUCLK切回HOSC/COREPLL
	*/
	uint32_t val;

	/* switch cpuclk src to 4MRC */
	val = sys_read32(CMU_SYSCLK);
	sys_write32(val & ~0x7, CMU_SYSCLK);

	/* update HOSC_CTL & wait HOSC_READY */
	// sys_write32(0x601f1b6 | (sys_read32(HOSC_CTL) & HOSC_CTL_HOSC_CAP_MASK), HOSC_CTL);
	// while(!(sys_read32(HOSC_CTL) & (1<< HOSC_CTL_HOSC_READY)));

	// When ATT uses USB transmission, hosc_ctl bit25 is set to 1, hosc will briefly stop, 
	// which will cause USB disconnection. So we can only not set hosc_ctl bit25, 
	// no longer query hosc_ready bit, but use delay to ensure hosc ready
	//sys_write32(0x401f1b6 | (sys_read32(HOSC_CTL) & HOSC_CTL_HOSC_CAP_MASK), HOSC_CTL);
	//mdelay(2);
	/*fix usb disconnected error*/
	sys_write32(0x401f736 | (sys_read32(HOSC_CTL) & HOSC_CTL_HOSC_CAP_MASK), HOSC_CTL); // 
	mdelay(2); 
	sys_write32(0x401f7f6 | (sys_read32(HOSC_CTL) & HOSC_CTL_HOSC_CAP_MASK), HOSC_CTL); //
	mdelay(2);


	/* backup cpuclk src to COREPLL */
	sys_write32(val, CMU_SYSCLK);
}

void soc_cmu_init(void)
{
	LOG_I("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));

	leopard_set_hosc_ctrl();
#if 0
	*((REG32)(HOSC_CTL)) &=~HOSC_CTL_HOSC_CAP_MASK;
	*((REG32)(HOSC_CTL)) |=(0x64<<HOSC_CTL_HOSC_CAP_SHIFT);
	*((REG32)(HOSC_CTL))  =( *((REG32)(HOSC_CTL))  & (~0xffff)) | (0x7730);
#endif

	LOG_I("22-HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));
}
