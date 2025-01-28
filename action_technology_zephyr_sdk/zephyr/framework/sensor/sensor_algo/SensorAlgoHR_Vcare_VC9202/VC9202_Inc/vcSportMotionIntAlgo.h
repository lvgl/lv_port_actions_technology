#ifndef VC_SPORT_MOTION_ALGO_H
#define VC_SPORT_MOTION_ALGO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


void vcSportMotionAlgoInit(void);
int32_t vcSportMotionCalculate(int32_t x, int32_t y, int32_t z);
char* version_info_SportMotion(void);

#endif
