/*
 Copyright (c) 1997-2025, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M debug functions interface based on DWT
 *
 */
#include <kernel.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <linker/linker-defs.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#if defined(CONFIG_PRINTK) || defined(CONFIG_LOG)
#define DBG_EXC(...) LOG_ERR(__VA_ARGS__)
#else
#define DBG_EXC(...)
#endif /* CONFIG_PRINTK || CONFIG_LOG */

#if (CONFIG_FAULT_DUMP == 2)
#define DBG_INFO(...) PR_EXC(__VA_ARGS__)
#else
#define DBG_INFO(...)
#endif

#if defined(CONFIG_CORTEX_M_DBG_DWT)

//Ref:https://interrupt.memfault.com/blog/cortex-m-watchpoints

typedef enum{
    DWT_FUNCTION_NONE = 0,
    DWT_FUNCTION_INST_WATCH = 4,
    DWT_FUNCTION_RWATCH = 5,
    DWT_FUNCTION_WWACTH = 6,
    DWT_FUNCTION_RWWATCH = 7,
    DWT_FUNCTION_DATAMATCH_WATCH = 16,
}DWT_FUNCTION_TYPE_E;

typedef struct {
  volatile u32_t COMP;
  volatile u32_t MASK;
  volatile u32_t FUNCTION;
  volatile u32_t RSVD;
} sDwtCompCfg;

void z_arm_dwt_dump(void)
{
    DBG_INFO("DWT Dump:");
    DBG_INFO(" DWT_CTRL=0x%x", DWT->CTRL);

    const size_t num_comparators = (DWT->CTRL>>28) & 0xF;
    DBG_INFO("	NUMCOMP=0x%x", num_comparators);

    for (size_t i = 0; i < num_comparators; i++) {
      const sDwtCompCfg *config = &DWT->COMP_CONFIG[i];

      DBG_INFO(" Comparator %d Config", (int)i);
      DBG_INFO("  0x%08x DWT_FUNC%d: 0x%08x",
                  &config->FUNCTION, (int)i, config->FUNCTION);
      DBG_INFO("  0x%08x DWT_COMP%d: 0x%08x",
                  &config->COMP, (int)i, config->COMP);
      DBG_INFO("  0x%08x DWT_MASK%d: 0x%08x",
                  &config->MASK, (int)i, config->MASK);
    }
}

void z_arm_dwt_reset(void)
{
    const size_t num_comparators = (DWT->CTRL>>28) & 0xF;

    /* Enable DWT & Debug monitor exception*/
    /* TRCENA: Global enable for all features configured and controlled by the DWT and ITM blocks.*/
    /* MON_EN: Enable the DebugMonitor exception */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk | CoreDebug_DEMCR_MON_EN_Msk;

    for (size_t i = 0; i < num_comparators; i++) {
        sDwtCompCfg *config = &DWT->COMP_CONFIG[i];
        config->FUNCTION = config->COMP = config->MASK = 0;
    }
}

void z_arm_dwt_install_watchpoint(int comp_id, u32_t func, u32_t comp, u32_t mask)
{
    const size_t num_comparators = (DWT->CTRL>>28) & 0xF;
    if (comp_id > num_comparators) {
        DBG_INFO("Invalid COMP_ID of %d", comp_id);
        return;
    }

    //for cortex-m4's data value matching is only supported in comparator 1
    if (func == DWT_FUNCTION_DATAMATCH_WATCH){
        comp_id = 1;
    }

    sDwtCompCfg *config = &DWT->COMP_CONFIG[comp_id];

    if (func != DWT_FUNCTION_DATAMATCH_WATCH){
        config->COMP = comp;
        config->MASK = mask;
        config->FUNCTION = func;
    }else{
        config->MASK = 0;
        config->COMP = comp;
        config->FUNCTION = mask;

    }
}
/**
 *
 * @brief watch addr data
 * z_ardm_dwt_set_addr_watchpoint add size type index
 * If watch type is 6,this routine  equivalent to GDB’s watch *(uint8_t[<size>] *)<address_to_watch>
   except that a helpful error is raised if the configuration requested is invalid due to the limitations
   we discussed above and that the system will always halt on the write access
   rather than only halting when a value changes.
 *
 * @Param addr: watch address
 * @Param size: watch size, max size is  2^size bytes, typecally is 0x1f, means 32Kbytes
 * @Param type: watch type
            0: watch none
            4: instrction fetch watchpoint
            5: read data watchpoint
            6: write data watchpoint
            7: read or write data watchpoint
 * @Param index: watch point index,  typecally max index is 3, means has 4 watchpoints
 */
void z_arm_dwt_set_addr_watchpoint(u32_t addr, u32_t size, u32_t type, u32_t index)
{
    int i, mask;

    if((addr % size)){
        DBG_INFO("Address(0x%x) needs to be aligned by size", addr);
        return;
    }

    if((size & (size - 1))){
        DBG_INFO("Size(0x%x) must be power of 2", size);
        return;
    }

    i = 0;
    mask = size;
    while(1){
        mask = mask >> 1;
        if(!mask){
            break;
        }
        i++;
    }

    mask = i;

    z_arm_dwt_install_watchpoint(index, type, addr, mask);
}

void z_arm_dwt_unset_addr_watchpoint(u32_t index)
{
    z_arm_dwt_install_watchpoint(index, DWT_FUNCTION_NONE, 0, 0);
}


void z_arm_dwt_set_data_value_watchpoint(u32_t value, u32_t size)
{
    u32_t mask;
    if(size != 1 && size != 2 && size != 4){
        DBG_INFO("Match watchpoint muste be 1,2,or 4bytes");
        return;
    }

    //for values with a size less than a word, the value must be copied
    //into the other parts of the comparator register
    if(size == 2){
        value = (value | (value << 16));
    }else if(size == 1){
        value = (value | (value << 8) | ((value << 16) | (value << 24)));
    }

    /*
    * The range scanned can be filtered with other comparators
    * or disabled by setting the value to the comparator being
    *used. For this example, let's just disable:
    *	DATAVADDR0 (bits[19:16]) = comp_id
    *	DATAVADDR1 (bits[15:12]) = comp_id
    *
    *Rest of Config:
    *	DATAVSIZE (bits[11:10]) = size >> 1
    *	DATAVMATCH (bit[8]) = 1
    *	 Encoding is 00 (byte), 01 (half-word), 10 (word)
    *	FUNCTION = 0x6 for a write-only watchpoint
    */
    mask = (1 << 16) | (1 << 12) | (1 << 8) | ((size >> 1) << 10) | DWT_FUNCTION_WWACTH;

    z_arm_dwt_install_watchpoint(1, DWT_FUNCTION_DATAMATCH_WATCH, value, mask);

}

#endif

#if defined(CONFIG_CORTEX_M_DBG_DPB)

//ref: https://interrupt.memfault.com/blog/cortex-m-debug-monitor

typedef struct {
    bool enabled;
    u32_t revision;
    size_t num_code_comparators;
    size_t num_literal_comparators;
} sFpbConfig;

typedef struct {
    volatile u32_t FP_CTRL;
    volatile u32_t FP_REMAP;
    // Number Implemented determined by FP_CTRL
    volatile u32_t FP_COMP[];
} sFpbUnit;

#define FPB  ((sFpbUnit *)0xE0002000)

static void prv_enable(bool do_enable)
{
    if (do_enable) {
        // clear any stale state in the DFSR
        SCB->DFSR = SCB->DFSR;
        CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;
    } else {
        CoreDebug->DEMCR &= ~(CoreDebug_DEMCR_MON_EN_Msk);
    }
}

static bool prv_halting_debug_enabled(void)
{
    volatile u32_t *dhcsr = (u32_t *)0xE000EDF0;
    return ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0);
}

bool debug_monitor_enable(void)
{
    if (prv_halting_debug_enabled()) {
        DBG_INFO("Halting Debug Enabled - "
                    "Can't Enable Monitor Mode Debug!");
        return false;
    }
    prv_enable(true);


    // Priority for DebugMonitor Exception is bits[7:0].
    // We will use the lowest priority so other ISRs can
    // fire while in the DebugMonitor Interrupt
    SCB->SHP[3] = 0xff;

    DBG_INFO("Monitor Mode Debug Enabled!");
    return true;
}

bool debug_monitor_disable(void)
{
    prv_enable(false);
    return true;
}

void fpb_dump_breakpoint_config(void) {
    const u32_t fp_ctrl = FPB->FP_CTRL;
    const u32_t fpb_enabled = fp_ctrl & 0x1;
    const u32_t revision = (fp_ctrl >> 28) & 0xF;
    const u32_t num_code_comparators =
        (((fp_ctrl >> 12) & 0x7) << 4) | ((fp_ctrl >> 4) & 0xF);

    DBG_INFO("FPB Revision: %d, Enabled: %d, Hardware Breakpoints: %d",
              revision, (int)fpb_enabled, (int)num_code_comparators);

    for (size_t i = 0; i < num_code_comparators; i++) {
        const u32_t fp_comp = FPB->FP_COMP[i];
        const bool enabled = fp_comp & 0x1;
        const u32_t replace = fp_comp >> 30;

        u32_t instruction_address = fp_comp & 0x1FFFFFFC;
        if (replace == 0x2) {
            instruction_address |= 0x2;
        }

        DBG_INFO("  FP_COMP[%d] Enabled %d, Replace: %d, Address 0x%x",
                    (int)i, (int)enabled, (int)replace, instruction_address);
    }
}


void fpb_disable(void)
{
    FPB->FP_CTRL = (FPB->FP_CTRL & ~0x3) | 0x2;
}

void fpb_enable(void)
{
    FPB->FP_CTRL |= 0x3;
}

void fpb_get_config(sFpbConfig *config) {
    u32_t fp_ctrl = FPB->FP_CTRL;

    const u32_t enabled = fp_ctrl & 0x1;
    const u32_t revision = (fp_ctrl >> 28) & 0xF;
    const u32_t num_code =
          (((fp_ctrl >> 12) & 0x7) << 4) | ((fp_ctrl >> 4) & 0xF);
    const u32_t num_lit = (fp_ctrl >> 8) & 0xF;

    *config = (sFpbConfig) {
        .enabled = enabled != 0,
        .revision = revision,
        .num_code_comparators = num_code,
        .num_literal_comparators = num_lit,
    };
}

bool fpb_set_breakpoint(size_t comp_id, u32_t instr_addr) {
    sFpbConfig config;
    fpb_get_config(&config);
    if (config.revision != 0) {
        DBG_INFO("Revision %d Parsing Not Supported", config.revision);
        return false;
    }

    const size_t num_comps = config.num_code_comparators;
    if (comp_id >= num_comps) {
        DBG_INFO("Instruction Comparator %d Not Implemented", num_comps);
        return false;
    }

    if (instr_addr >= 0x20000000) {
        DBG_INFO("Address 0x%x is not in code region", instr_addr);
        return false;
    }

    if (!config.enabled) {
        DBG_INFO("Enabling FPB.");
        fpb_enable();
    }


    const u32_t replace = (instr_addr & 0x2) == 0 ? 1 : 2;
    const u32_t fp_comp = (instr_addr & ~0x3) | 0x1 | (replace << 30);
    FPB->FP_COMP[comp_id] = fp_comp;
    return true;
}

bool fpb_get_comp_config(size_t comp_id, sFpbCompConfig *comp_config) {
    sFpbConfig config;
    fpb_get_config(&config);
    if (config.revision != 0) {
        DBG_INFO("Revision %d Parsing Not Supported", config.revision);
        return false;
    }

    const size_t num_comps = config.num_code_comparators + config.num_literal_comparators;
    if (comp_id >= num_comps) {
        DBG_INFO("Comparator %d Not Implemented", num_comps);
        return false;
    }

    u32_t fp_comp = FPB->FP_COMP[comp_id];
    bool enabled = fp_comp & 0x1;
    u32_t replace = fp_comp >> 30;

    u32_t address = fp_comp & 0x1FFFFFFC;
    if (replace == 0x2) {
        address |= 0x2;
    }

      *comp_config = (sFpbCompConfig) {
        .enabled = enabled,
        .replace = replace,
        .address = address,
      };
    return true;
}

#endif

