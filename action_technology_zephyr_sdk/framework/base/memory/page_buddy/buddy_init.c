#include <kernel.h>
#include "heap.h"
#include "page_inner.h"

#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
extern void mpu_enable_region(unsigned int index);
extern void mpu_disable_region(unsigned int index);
#endif

rom_buddy_data_t g_rom_buddy_data;

static void buddy_halt(void)
{
	//panic("memory buddy_error");
}

void buddy_rom_data_init(void)
{
#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
    mpu_disable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX); 
#endif

    g_rom_buddy_data.printf = (void *)os_printk;
    g_rom_buddy_data.halt = buddy_halt;
    g_rom_buddy_data.pagepool_convert_index_to_addr = pagepool_convert_index_to_addr;

#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
    mpu_enable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX); 
#endif	
}
