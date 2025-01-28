#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include "spi_flash.h"

#define NOR_STATUS1_MASK (0x1f<<2)  /*bp4-bp0  status1 bit6-bit2*/
#define NOR_STATUS2_MASK (0x1<<6)  /*cmp   status2  bit6*/

/**********ATS3085 NOR GD25Q16E  2MB  ****************/
#define ATS3085_NOR_CHIPID	0x1560c8  	/*GD25Q16E*/
#define ATS3085_CMP0_VAL		0
#define ATS3085_CMP0_PROTECT_16KB	0x1b /*0-16KB*/
#define ATS3085_CMP0_PROTECT_64KB	0x9  /*0-64KB*/
#define ATS3085_CMP0_PROTECT_128KB	0xa  /*0-128KB*/
#define ATS3085_CMP0_PROTECT_256KB	0xb  /*0-256KB*/
#define ATS3085_CMP0_PROTECT_512KB	0xc  /*0-512KB*/
#define ATS3085_CMP0_PROTECT_1MB 	0xd  /*0-1MB*/
#define ATS3085_CMP0_PROTECT_2MB 	0xe  /*0-2MB*/
#define ATS3085_CMP1_VAL			1
#define ATS3085_CMP1_PROTECT_1984KB	0x1  /*0- (2MB-64kb)*/
#define ATS3085_CMP1_PROTECT_1920KB	0x2  /*0- (2MB-128kb)*/
#define ATS3085_CMP1_PROTECT_1792KB	0x3  /*0- (2MB-256kb)*/
#define ATS3085_CMP1_PROTECT_1536KB	0x4  /*0- (2MB-512kb)*/

#define ATS3085_NOR_BP_STATUS1 (ATS3085_CMP0_PROTECT_16KB<<2)
#define ATS3085_NOR_CMP_STATUS2 (ATS3085_CMP0_VAL<<6)


/**********GD25Q256E(3085c) 32MB****************/
#define GD25Q256E_NOR_CHIPID	0x1940c8  	/*GD25Q256E*/
#define GD25Q256E_PROTECT_64KB	0x11  		/*0-64KB*/
#define GD25Q256E_PROTECT_128KB	0x12  		/*0-128KB*/
#define GD25Q256E_PROTECT_256KB	0x13  		/*0-256KB*/
#define GD25Q256E_PROTECT_512KB	0x14  		/*0-512KB*/
#define GD25Q256E_PROTECT_1MB 	0x15  		/*0-1MB*/
#define GD25Q256E_PROTECT_2MB 	0x16  		/*0-2MB*/
#define GD25Q256E_PROTECT_4MB 	0x17  		/*0-4MB*/
#define GD25Q256E_PROTECT_8MB 	0x18  		/*0-8MB*/
#define GD25Q256E_PROTECT_16MB 	0x19  		/*0-16MB*/
#define GD25Q256E_PROTECT_32MB 	0x1A  		/*0-32MB*/

#define GD25Q256E_STATUS1_MASK  	(NOR_STATUS1_MASK|(1<<7))
#define GD25Q256E_NOR_BP_STATUS1	(GD25Q256E_PROTECT_64KB<<2)
#define GD25Q256E_NOR_STATUS2  		(0<<6)



/**********GD25LF32E(3089c) 4MB****************/
#define GD25LF32E_NOR_CHIPID	0x1663c8  	/*GD25LF32E*/
#define GD25LF32E_CMP0_VAL		0

#define GD25LF32E_CMP0_PROTECT_4KB		0x19 /*0-4KB*/
#define GD25LF32E_CMP0_PROTECT_8KB		0x1a /*0-8KB*/
#define GD25LF32E_CMP0_PROTECT_16KB		0x1b /*0-16KB*/
#define GD25LF32E_CMP0_PROTECT_32KB		0x1c /*0-32KB*/
#define GD25LF32E_CMP0_PROTECT_64KB		0x9  /*0-64KB*/
#define GD25LF32E_CMP0_PROTECT_128KB	0xa  /*0-128KB*/
#define GD25LF32E_CMP0_PROTECT_256KB	0xb  /*0-256KB*/
#define GD25LF32E_CMP0_PROTECT_512KB	0xc  /*0-512KB*/
#define GD25LF32E_CMP0_PROTECT_1MB 		0xd  /*0-1MB*/
#define GD25LF32E_CMP0_PROTECT_2MB 		0xe  /*0-2MB*/
#define GD25LF32E_CMP0_PROTECT_4MB 		0xf  /*0-4MB*/


#define GD25LF32E_NOR_BP_STATUS1	(GD25LF32E_CMP0_PROTECT_16KB<<2)
#define GD25LF32E_NOR_STATUS2  		(GD25LF32E_CMP0_VAL<<6)


/**********GD25LE64E(3089) 8MB****************/
#define GD25LE64E_NOR_CHIPID	0x1760c8  	/*GD25LF32E*/
#define GD25LE64E_CMP0_VAL		0

#define GD25LE64E_CMP0_PROTECT_4KB		0x19 /*0-4KB*/
#define GD25LE64E_CMP0_PROTECT_8KB		0x1a /*0-8KB*/
#define GD25LE64E_CMP0_PROTECT_16KB		0x1b /*0-16KB*/
#define GD25LE64E_CMP0_PROTECT_32KB		0x1c /*0-32KB*/
#define GD25LE64E_CMP0_PROTECT_128KB	0x9  /*0-128KB*/
#define GD25LE64E_CMP0_PROTECT_256KB	0xa  /*0-256KB*/
#define GD25LE64E_CMP0_PROTECT_512KB	0xb  /*0-512KB*/
#define GD25LE64E_CMP0_PROTECT_1MB 		0xc  /*0-1MB*/
#define GD25LE64E_CMP0_PROTECT_2MB 		0xd  /*0-2MB*/
#define GD25LE64E_CMP0_PROTECT_4MB 		0xe  /*0-4MB*/
#define GD25LE64E_CMP0_PROTECT_8MB 		0xf  /*0-8MB*/


#define GD25LE64E_NOR_BP_STATUS1	(GD25LE64E_CMP0_PROTECT_16KB<<2)
#define GD25LE64E_NOR_STATUS2  		(GD25LE64E_CMP0_VAL<<6)




struct nor_wp_info {
	unsigned int chipid;
	unsigned char wp_status1_val;
	unsigned char wp_status1_mask;
	unsigned char wp_status2_val;
	unsigned char wp_status2_mask;
};
const struct nor_wp_info  g_nor_wp_info[] = {
	{ATS3085_NOR_CHIPID, ATS3085_NOR_BP_STATUS1, NOR_STATUS1_MASK,  ATS3085_NOR_CMP_STATUS2, NOR_STATUS2_MASK},
	{GD25Q256E_NOR_CHIPID, GD25Q256E_NOR_BP_STATUS1, GD25Q256E_STATUS1_MASK,  GD25Q256E_NOR_STATUS2, NOR_STATUS2_MASK},
	{GD25LF32E_NOR_CHIPID, GD25LF32E_NOR_BP_STATUS1, NOR_STATUS1_MASK,  GD25LF32E_NOR_STATUS2, NOR_STATUS2_MASK},
	{GD25LE64E_NOR_CHIPID, GD25LE64E_NOR_BP_STATUS1, NOR_STATUS1_MASK,  GD25LE64E_NOR_STATUS2, NOR_STATUS2_MASK},
};

static __ramfunc void xspi_nor_write_status(struct spinor_info *sni, u8_t status1, u8_t status2)
{
	u8_t status[2];
	unsigned int flag;
	status[0] = status1;
	status[1] = status2;
	flag = sni->spi.flag;
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  //wait ready
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2, &status2, 1);
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,  &status1,  1);
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,  status, 2);
	sni->spi.flag =  flag;
}

static __ramfunc void xspi_nor_read_status(struct spinor_info *sni, u8_t *status1, u8_t *status2)
{
	*status1 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	*status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
}

static __ramfunc void xspi_nor_protect_handle(struct spinor_info *sni, const struct nor_wp_info *wp, bool benable)
{
	u32_t flags;
	u8_t status1, status2;
	u8_t val1;
	flags = irq_lock();
	xspi_nor_read_status(sni, &status1, &status2);
//#ifdef CONFIG_XSPI_NOR_ACTS_DUMP_INFO
	printk("status1-2=0x%x,0x%x\n", status1, status2);
//#endif
	val1 = wp->wp_status1_mask & status1;
	if(benable){
		if(val1 !=  wp->wp_status1_val){// enable protect
			status1 =  ((~wp->wp_status1_mask) & status1) | wp->wp_status1_val;
			status2 =  ((~wp->wp_status2_mask) & status2) | wp->wp_status2_val;
			printk("enable status1-2=0x%x,0x%x\n", status1, status2);
			xspi_nor_write_status(sni, status1, status2);
		}
	}else{
		if(val1 !=	0){// diable protect
			status1 =  (~wp->wp_status1_mask) & status1;
			status2 =  (~wp->wp_status2_mask) & status2;
			printk("disable status1-2=0x%x,0x%x\n", status1, status2);
			xspi_nor_write_status(sni, status1, status2);
		}
	}
//#ifdef CONFIG_XSPI_NOR_ACTS_DUMP_INFO
	xspi_nor_read_status(sni, &status1, &status2);
	printk("read again status1-2=0x%x,0x%x\n", status1, status2);
//#endif
	irq_unlock(flags);

}


int nor_write_protection(const struct device *dev, bool enable)
{
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;
	const struct nor_wp_info *wp;
	int i;
	for(i = 0; i < ARRAY_SIZE(g_nor_wp_info); i++){
		wp = &g_nor_wp_info[i];
		if(wp->chipid == sni->chipid)
			xspi_nor_protect_handle(sni, wp, enable);
	}
	return 0;
}


