/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-12-PM12:48:04             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_memctrl.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-12-PM12:48:04
 *******************************************************************************/

#ifndef SOC_MEMCTRL_H_
#define SOC_MEMCTRL_H_


//--------------MemoryController-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     MemoryController_BASE                                             0x40010000
#define     MEMORYCTL                                                         (MemoryController_BASE+0x00000000)
#define     CPU_ERROR_ADDR                                                    (MemoryController_BASE+0x00000008)
#define     MEMORYCTL2                                                        (MemoryController_BASE+0x00000020)
#define     MEMORYCTL3                                                        (MemoryController_BASE+0x00000024)
#define     DSP_ERROR_ADDR                                                    (MemoryController_BASE+0x00000030)
#define     DBGCTL                                                            (MemoryController_BASE+0x00000040)
#define     DSPPAGEADDR0                                                      (MemoryController_BASE+0x00000080)
#define     DSPPAGEADDR1                                                      (MemoryController_BASE+0x00000084)
#define     DSPPAGEADDR2                                                      (MemoryController_BASE+0x00000088)
#define     DSPPAGEADDR3                                                      (MemoryController_BASE+0x0000008c)
#define     MPUIE                                                             (MemoryController_BASE+0x00000100)
#define     MPUIP                                                             (MemoryController_BASE+0x00000104)
#define     MPUCTL0                                                           (MemoryController_BASE+0x00000110)
#define     MPUBASE0                                                          (MemoryController_BASE+0x00000114)
#define     MPUEND0                                                           (MemoryController_BASE+0x00000118)
#define     MPUERRADDR0                                                       (MemoryController_BASE+0x0000011c)
#define     MPUCTL1                                                           (MemoryController_BASE+0x00000120)
#define     MPUBASE1                                                          (MemoryController_BASE+0x00000124)
#define     MPUEND1                                                           (MemoryController_BASE+0x00000128)
#define     MPUERRADDR1                                                       (MemoryController_BASE+0x0000012c)
#define     MPUCTL2                                                           (MemoryController_BASE+0x00000130)
#define     MPUBASE2                                                          (MemoryController_BASE+0x00000134)
#define     MPUEND2                                                           (MemoryController_BASE+0x00000138)
#define     MPUERRADDR2                                                       (MemoryController_BASE+0x0000013c)
#define     MPUCTL3                                                           (MemoryController_BASE+0x00000140)
#define     MPUBASE3                                                          (MemoryController_BASE+0x00000144)
#define     MPUEND3                                                           (MemoryController_BASE+0x00000148)
#define     MPUERRADDR3                                                       (MemoryController_BASE+0x0000014c)
#define     BIST_EN0                                                          (MemoryController_BASE+0x00000200)
#define     BIST_FIN0                                                         (MemoryController_BASE+0x00000204)
#define     BIST_INFO0                                                        (MemoryController_BASE+0x00000208)
#define     BIST_EN1                                                          (MemoryController_BASE+0x0000020c)
#define     BIST_FIN1                                                         (MemoryController_BASE+0x00000210)
#define     BIST_INFO1                                                        (MemoryController_BASE+0x00000214)
#define     SPI_CACHE_MAPPING_ADDR0                                           (MemoryController_BASE+0x00000300)
#define     SPI_CACHE_ADDR0_ENTRY                                             (MemoryController_BASE+0x00000304)
#define     SPI_CACHE_MAPPING_ADDR1                                           (MemoryController_BASE+0x00000308)
#define     SPI_CACHE_ADDR1_ENTRY                                             (MemoryController_BASE+0x0000030c)
#define     SPI_CACHE_MAPPING_ADDR2                                           (MemoryController_BASE+0x00000310)
#define     SPI_CACHE_ADDR2_ENTRY                                             (MemoryController_BASE+0x00000314)
#define     SPI_CACHE_MAPPING_ADDR3                                           (MemoryController_BASE+0x00000318)
#define     SPI_CACHE_ADDR3_ENTRY                                             (MemoryController_BASE+0x0000031c)
#define     SPI_CACHE_MAPPING_ADDR4                                           (MemoryController_BASE+0x00000320)
#define     SPI_CACHE_ADDR4_ENTRY                                             (MemoryController_BASE+0x00000324)
#define     SPI_CACHE_MAPPING_ADDR5                                           (MemoryController_BASE+0x00000328)
#define     SPI_CACHE_ADDR5_ENTRY                                             (MemoryController_BASE+0x0000032c)
#define     SPI_CACHE_MAPPING_ADDR6                                           (MemoryController_BASE+0x00000330)
#define     SPI_CACHE_ADDR6_ENTRY                                             (MemoryController_BASE+0x00000334)
#define     SPI_CACHE_MAPPING_ADDR7                                           (MemoryController_BASE+0x00000338)
#define     SPI_CACHE_ADDR7_ENTRY                                             (MemoryController_BASE+0x0000033c)

#define     CACHE_MAPPING_ITEM_NUM			(8)


#define     MEMORYCTL_BUSERROR_BIT          BIT(3)

#define     DISPLAY_MMU_CTL 	                                              (MemoryController_BASE+0x00000a00)
#define     DISPLAY_MMU_BASEADDR0                                             (MemoryController_BASE+0x00000a10)
#define     DISPLAY_MMU_BASEADDR1                                             (MemoryController_BASE+0x00000a14)
#define     DISPLAY_MMU_BASEADDR2                                             (MemoryController_BASE+0x00000a18)
#define     DISPLAY_MMU_BASEADDR3                                             (MemoryController_BASE+0x00000a1c)

typedef struct {
	volatile unsigned int mapping_addr;
	volatile unsigned int mapping_entry;
} cache_mapping_register_t;

typedef struct
{
    volatile uint32_t MPUCTL;
    volatile uint32_t MPUBASE;
    volatile uint32_t MPUEND;
    volatile uint32_t MPUERRADDR;
}mpu_base_register_t;

void soc_memctrl_set_mapping(int idx, u32_t cpu_addr, u32_t nor_bus_addr);
int soc_memctrl_mapping(u32_t cpu_addr, u32_t nor_phy_addr, int enable_crc);
int soc_memctrl_unmapping(u32_t map_addr);
void *soc_memctrl_create_temp_mapping(u32_t nor_phy_addr, int enable_crc);
void soc_memctrl_clear_temp_mapping(void *cpu_addr);
u32_t soc_memctrl_cpu_to_nor_phy_addr(u32_t cpu_addr);

void soc_memctrl_config_cache_size(int cache_size_mode);
void soc_memctrl_cache_invalid(void);

static inline void soc_memctrl_set_dsp_mapping(int idx, u32_t map_addr,
					  u32_t phy_addr)
{
    return;
}

static inline u32_t soc_memctrl_cpu_addr_to_bus_addr(u32_t cpu_addr)
{
	u32_t bus_addr;

	if (cpu_addr < 0x10000000)
		bus_addr = 0x10000000;
	else
		bus_addr = cpu_addr & ~0xe0000000;

	return bus_addr;
}

#endif /* SOC_MEMCTRL_H_ */
