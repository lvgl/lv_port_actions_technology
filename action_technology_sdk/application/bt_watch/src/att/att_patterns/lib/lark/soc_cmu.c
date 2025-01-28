#include <gcc_include.h>
#include <soc.h>
#include <att_debug.h>

#define HOSC_CTL_HOSCI_CAP(x)	 ((x) << HOSC_CTL_HOSCI_CAP_SHIFT)
#define HOSC_CTL_HOSCO_CAP(x)	 ((x) << HOSC_CTL_HOSCO_CAP_SHIFT)

/*!
 * \brief get the HOSC capacitance
 * \n  capacitance is x10 fixed value
 */
extern int soc_get_hosc_cap(void)
{
    int  cap = 0;

    uint32_t  hosc_ctl = sys_read32(HOSCLDO_CTL);

    cap = (hosc_ctl >> 16) & 0xff;

    return cap;
}


/*!
 * \brief set the HOSC capacitance
 * \n  capacitance is x10 fixed value
 */
extern void soc_set_hosc_cap(int cap)
{
	int val, loop = 400;
    LOG_I("set hosc cap:%d.%d pf\n", cap / 10, cap % 10);

	val = sys_read32(HOSCLDO_CTL);
	val &= 0xFFFF;
	val |= (cap << 16);
	sys_write32(val, HOSCLDO_CTL);
	while(loop)loop--;
	val |= (0x1 << 24);
	sys_write32(val, HOSCLDO_CTL);		/* bit16-bit23 same to HOSC_CTL bit16-bit23 */

	val = sys_read32(HOSC_CTL) & ~(HOSC_CTL_HOSCI_CAP_MASK | HOSC_CTL_HOSCO_CAP_MASK);
	val |= HOSC_CTL_HOSCI_CAP(cap) | HOSC_CTL_HOSCO_CAP(cap);
	sys_write32(val, HOSC_CTL);

    udelay(100);

	LOG_I("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));

}

void soc_cmu_init(void)
{
	int val;

	sys_write32(0x0164000C, HOSCLDO_CTL);

	val = sys_read32(HOSC_CTL) & (HOSC_CTL_HOSCI_CAP_MASK | HOSC_CTL_HOSCO_CAP_MASK);
	val |= 0x7030;
	sys_write32(val, HOSC_CTL);

	LOG_I("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));

}

