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


#if 0
#define     MEMORYCTL                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000000)
#define     ALTERNATEINSTR0                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000004)
#define     ALTERNATEINSTR1                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000008)
#define     ALTERNATEINSTR2                                                   (MEMORY_CONTROLLER_REG_BASE+0x0000000c)
#define     ALTERNATEINSTR3                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000010)
#define     ALTERNATEINSTR4                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000014)
#define     ALTERNATEINSTR5                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000018)
#define     ALTERNATEINSTR6                                                   (MEMORY_CONTROLLER_REG_BASE+0x0000001c)
#define     ALTERNATEINSTR7                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000020)
#define     ALTERNATEINSTR8                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000024)
#define     ALTERNATEINSTR9                                                   (MEMORY_CONTROLLER_REG_BASE+0x00000028)
#define     ALTERNATEINSTR10                                                  (MEMORY_CONTROLLER_REG_BASE+0x0000002c)
#define     ALTERNATEINSTR11                                                  (MEMORY_CONTROLLER_REG_BASE+0x00000030)
#define     ALTERNATEINSTR12                                                  (MEMORY_CONTROLLER_REG_BASE+0x00000034)
#define     ALTERNATEINSTR13                                                  (MEMORY_CONTROLLER_REG_BASE+0x00000038)
#define     ALTERNATEINSTR14                                                  (MEMORY_CONTROLLER_REG_BASE+0x0000003c)
#define     ALTERNATEINSTR15                                                  (MEMORY_CONTROLLER_REG_BASE+0x00000040)
#define     FIXADDR0                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000044)
#define     FIXADDR1                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000048)
#define     FIXADDR2                                                          (MEMORY_CONTROLLER_REG_BASE+0x0000004c)
#define     FIXADDR3                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000050)
#define     FIXADDR4                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000054)
#define     FIXADDR5                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000058)
#define     FIXADDR6                                                          (MEMORY_CONTROLLER_REG_BASE+0x0000005c)
#define     FIXADDR7                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000060)
#define     FIXADDR8                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000064)
#define     FIXADDR9                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000068)
#define     FIXADDR10                                                         (MEMORY_CONTROLLER_REG_BASE+0x0000006c)
#define     FIXADDR11                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000070)
#define     FIXADDR12                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000074)
#define     FIXADDR13                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000078)
#define     FIXADDR14                                                         (MEMORY_CONTROLLER_REG_BASE+0x0000007c)
#define     FIXADDR15                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000080)
#define     BIST_EN                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000084)
#define     BIST_FIN                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000088)
#define     BIST_INFO                                                         (MEMORY_CONTROLLER_REG_BASE+0x0000008c)
#define     MAPPING_ADDR0                                                     (MEMORY_CONTROLLER_REG_BASE+0x00000090)
#define     ADDR0_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x00000094)
#define     MAPPING_ADDR1                                                     (MEMORY_CONTROLLER_REG_BASE+0x00000098)
#define     ADDR1_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x0000009c)
#define     MAPPING_ADDR2                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000a0)
#define     ADDR2_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000a4)
#define     MAPPING_ADDR3                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000a8)
#define     ADDR3_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000ac)
#define     MAPPING_ADDR4                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000b0)
#define     ADDR4_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000b4)
#define     MAPPING_ADDR5                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000b8)
#define     ADDR5_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000bc)
#define     MAPPING_ADDR6                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000c0)
#define     ADDR6_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000c4)
#define     MAPPING_ADDR7                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000c8)
#define     ADDR7_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000cc)
#define     MAPPING_ADDR8                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000d0)
#define     ADDR8_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000d4)
#define     MAPPING_ADDR9                                                     (MEMORY_CONTROLLER_REG_BASE+0x000000d8)
#define     ADDR9_ENTRY                                                       (MEMORY_CONTROLLER_REG_BASE+0x000000dc)
#define     MAPPING_ADDR10                                                    (MEMORY_CONTROLLER_REG_BASE+0x000000e0)
#define     ADDR10_ENTRY                                                      (MEMORY_CONTROLLER_REG_BASE+0x000000e4)
#define     MAPPING_ADDR11                                                    (MEMORY_CONTROLLER_REG_BASE+0x000000e8)
#define     ADDR11_ENTRY                                                      (MEMORY_CONTROLLER_REG_BASE+0x000000ec)
#define     MAPPING_ADDR12                                                    (MEMORY_CONTROLLER_REG_BASE+0x000000f0)
#define     ADDR12_ENTRY                                                      (MEMORY_CONTROLLER_REG_BASE+0x000000f4)
#define     MAPPING_ADDR13                                                    (MEMORY_CONTROLLER_REG_BASE+0x000000f8)
#define     ADDR13_ENTRY                                                      (MEMORY_CONTROLLER_REG_BASE+0x000000fc)
#define     MAPPING_ADDR14                                                    (MEMORY_CONTROLLER_REG_BASE+0x00000100)
#define     ADDR14_ENTRY                                                      (MEMORY_CONTROLLER_REG_BASE+0x00000104)
#define     MAPPING_ADDR15                                                    (MEMORY_CONTROLLER_REG_BASE+0x00000108)
#define     ADDR15_ENTRY                                                      (MEMORY_CONTROLLER_REG_BASE+0x0000010c)
#define     DATA_ADDRESS_ERROR_INFO                                           (MEMORY_CONTROLLER_REG_BASE+0x00000110)
#define     INSTRUCTION_ADDRESS_ERROR_INFO                                    (MEMORY_CONTROLLER_REG_BASE+0x00000114)
#define     SYMBOL_TABLE_PTR0                                                 (MEMORY_CONTROLLER_REG_BASE+0x00000118)
#define     SYMBOL_TABLE_PTR1                                                 (MEMORY_CONTROLLER_REG_BASE+0x0000011c)
#define     SYMBOL_TABLE_PTR2                                                 (MEMORY_CONTROLLER_REG_BASE+0x00000120)
#define     SYMBOL_TABLE_PTR3                                                 (MEMORY_CONTROLLER_REG_BASE+0x00000124)
#define     MPUIE_LOW                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000140)
#define     MPUIE_HIGH                                                        (MEMORY_CONTROLLER_REG_BASE+0x00000144)
#define     MPUIP_LOW                                                         (MEMORY_CONTROLLER_REG_BASE+0x00000148)
#define     MPUIP_HIGH                                                        (MEMORY_CONTROLLER_REG_BASE+0x0000014c)
#define     MPUCTL0                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000150)
#define     MPUBASE0                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000154)
#define     MPUEND0                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000158)
#define     MPUERRADDR0                                                       (MEMORY_CONTROLLER_REG_BASE+0x0000015c)
#define     MPUCTL1                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000160)
#define     MPUBASE1                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000164)
#define     MPUEND1                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000168)
#define     MPUERRADDR1                                                       (MEMORY_CONTROLLER_REG_BASE+0x0000016c)
#define     MPUCTL2                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000170)
#define     MPUBASE2                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000174)
#define     MPUEND2                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000178)
#define     MPUERRADDR2                                                       (MEMORY_CONTROLLER_REG_BASE+0x0000017c)
#define     MPUCTL3                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000180)
#define     MPUBASE3                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000184)
#define     MPUEND3                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000188)
#define     MPUERRADDR3                                                       (MEMORY_CONTROLLER_REG_BASE+0x0000018c)
#define     MPUCTL4                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000190)
#define     MPUBASE4                                                          (MEMORY_CONTROLLER_REG_BASE+0x00000194)
#define     MPUEND4                                                           (MEMORY_CONTROLLER_REG_BASE+0x00000198)
#define     MPUERRADDR4                                                       (MEMORY_CONTROLLER_REG_BASE+0x0000019c)
#define     MPUCTL5                                                           (MEMORY_CONTROLLER_REG_BASE+0x000001a0)
#define     MPUBASE5                                                          (MEMORY_CONTROLLER_REG_BASE+0x000001a4)
#define     MPUEND5                                                           (MEMORY_CONTROLLER_REG_BASE+0x000001a8)
#define     MPUERRADDR5                                                       (MEMORY_CONTROLLER_REG_BASE+0x000001ac)
#define     MPUCTL6                                                           (MEMORY_CONTROLLER_REG_BASE+0x000001b0)
#define     MPUBASE6                                                          (MEMORY_CONTROLLER_REG_BASE+0x000001b4)
#define     MPUEND6                                                           (MEMORY_CONTROLLER_REG_BASE+0x000001b8)
#define     MPUERRADDR6                                                       (MEMORY_CONTROLLER_REG_BASE+0x000001bc)
#define     MPUCTL7                                                           (MEMORY_CONTROLLER_REG_BASE+0x000001c0)
#define     MPUBASE7                                                          (MEMORY_CONTROLLER_REG_BASE+0x000001c4)
#define     MPUEND7                                                           (MEMORY_CONTROLLER_REG_BASE+0x000001c8)
#define     MPUERRADDR7                                                       (MEMORY_CONTROLLER_REG_BASE+0x000001cc)
#define     CACHE_CONFLICT_ADDR                                               (MEMORY_CONTROLLER_REG_BASE+0x00000200)


#define     CACHE_MAPPING_REGISTER_BASE	    (MAPPING_ADDR0)

#define     CACHE_MAPPING_ITEM_NUM			(16)

#define     MEMORYCTL_BUSERROR_BIT          BIT(18)

#define     CACHE_LINE_SIZE                 32
#define     CACHE_CRC_LINE_SIZE             34

#define     CACHE_SIZE_32KB                 (0)
#define     CACHE_SIZE_16KB                 (1)

#endif


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
