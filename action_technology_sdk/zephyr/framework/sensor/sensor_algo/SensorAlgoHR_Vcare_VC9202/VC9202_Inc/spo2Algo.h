#ifndef SPO2_ALGO_H
#define SPO2_ALGO_H

#include <stdint.h>

void spo2AlgoInit(void);
int32_t spo2Algo(int32_t r_ppg, int32_t ir_ppg, int32_t mode);

void spo2_algo_content_init(void);

char *spo2version_info(void);

#endif
