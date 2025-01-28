#ifndef __SYS_IO_H__
#define __SYS_IO_H__

/**
 * \brief  actions classic style system io operations
 */

typedef volatile unsigned int *REG32;
typedef volatile unsigned short *REG16;
typedef volatile unsigned char *REG8;

#define act_writeb(val, reg)        ((*((volatile unsigned char *)(reg))) = (val))
#define act_writew(val, reg)        ((*((volatile unsigned short *)(reg))) = (val))
#define act_writel(val, reg)        ((*((volatile unsigned int *)(reg))) = (val))
#define act_readb(port)             (*((volatile unsigned char *)(port)))
#define act_readw(port)             (*((volatile unsigned short *)(port)))
#define act_readl(port)             (*((volatile unsigned int *)(port)))

/******************************************************************************/

static inline void sys_writeb(unsigned char val, unsigned long reg)
{
	*(volatile unsigned char *)(reg) = val;
}

static inline void sys_writew(unsigned short val, unsigned long reg)
{
	*(volatile unsigned short *)(reg) = val;
}

static inline void sys_writel(unsigned int val, unsigned long reg)
{
	*(volatile unsigned int *)(reg) = val;
}

static inline void sys_write32(unsigned int val, unsigned int reg)
{
	*(volatile unsigned int *)(reg) = val;
}

/******************************************************************************/

static inline unsigned char sys_readb(unsigned long reg)
{
	return (*(volatile unsigned char *)(reg));
}

static inline unsigned short sys_readw(unsigned long reg)
{
	return (*(volatile unsigned short *)(reg));
}

static inline unsigned int sys_readl(unsigned long reg)
{
	return (*(volatile unsigned int *)(reg));
}

static inline unsigned int sys_read32(unsigned int reg)
{
	return *(volatile unsigned int *)(reg);
}

/******************************************************************************/

static inline void sys_clrsetbits(unsigned long reg, unsigned int clear,
		unsigned int set)
{
	unsigned int val;

	val = sys_readl(reg);
	val &= ~clear;
	val |= set;
	sys_writel(val, reg);
}

static inline void sys_clrbits(unsigned long reg, unsigned int clear)
{
	unsigned int val;

	val = sys_readl(reg);
	val &= ~clear;
	sys_writel(val, reg);
}

static inline void sys_setbits(unsigned long reg, unsigned int set)
{
	unsigned int val;

	val = sys_readl(reg);
	val |= set;
	sys_writel(val, reg);
}

#endif /* __SYS_IO_H__ */
