#ifndef __SIMULATE_SIGNAL_H__
#define __SIMULATE_SIGNAL_H__

#include <stdint.h>
#include <stddef.h>

typedef int (*fn_simulate_action)(int, int, int, float*);
typedef struct {
    fn_simulate_action  action;
    int                 value;
    int                 param1;
    int                 param2;
    float               *out_data;
} simulate_action;

typedef struct {
    simulate_action     *actions;
} time_axis_data;

int simulate_pedo_moving_time(int duration_s, int freq_m, int reserve, float *out_data);
int simulate_pedo_moving_step(int step, int freq_m, int reserve, float *out_data);
int simulate_no_move_time(int duration_s, int reserve1, int reserve2, float *out_data);
int simulate_static_time(int duration_s, int reserve1, int reserve2, float *out_data);
int simulate_mix_no_move_handupdown(int duration_s, int reserve1, int reserve2, float *inout_data);
int simulate_mix_moving_handupdown(int duration_s, int reserve1, int reserve2, float *inout_data);

void init_simulate_time_axis(void);
int simulate_time_axis(time_axis_data *data, float *out_data);
uint64_t simulate_fake_get_time_ns(void);

#endif
