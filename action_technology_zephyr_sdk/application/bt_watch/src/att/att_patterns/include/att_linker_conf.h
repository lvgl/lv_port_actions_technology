#ifndef __ATT_LINKER_CONF_H
#define __ATT_LINKER_CONF_H

#define __ENTRY_CODE __attribute__((section(".entry")))



/* Used by _bss_zero or arch-specific implementation */

#if defined(__ARMCOMPILER_VERSION)

/* Fix for AC-6.9 */
#ifndef __GNUC__
#define __GNUC__	9
#endif

//compile by armclang
#define __bss_start				Image$$ER_BSS$$Base
#define __bss_end				Image$$ER_BSS$$ZI$$Limit

#endif

extern char __bss_start[];
extern char __bss_end[];

#endif

