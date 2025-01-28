/*
 *
 *  Created on: 2023-6-04
 *      Author: runningHorse
 */

#ifndef BIODETECT
#define BIODETECT

#include <stdbool.h>
#include <stdint.h>

void f_BIOInit(int32_t gapDeltaBIO, int32_t gapDeltaPS);

uint8_t f_BIOdetect(int32_t inputBIO, int32_t inputPS, int32_t PSth);

char* version_info_Bio(void);

#endif /* BIODETECT */