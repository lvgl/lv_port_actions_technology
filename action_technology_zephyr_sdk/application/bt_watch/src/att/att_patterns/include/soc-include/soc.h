#ifndef __SOC_H
#define __SOC_H

#define __FPU_PRESENT			1
// Interaction RAM
#define INTER_RAM_ADDR				(0x01068000)
#define INTER_RAM_SIZE				(0x00008000)

#include <gcc_include.h>
#include <soc_cmu.h>
#include <sys_io.h>
#include <soc_regs.h>
#include <cmsis.h>
#include <mp_btc.h>

#endif
