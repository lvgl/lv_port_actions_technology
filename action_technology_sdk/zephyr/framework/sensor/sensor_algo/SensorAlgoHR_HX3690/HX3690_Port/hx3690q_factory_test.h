#ifndef _HX3690L_FACTORY_TEST_H_
#define _HX3690L_FACTORY_TEST_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	LEAK_LIGHT_TEST = 1,
	GRAY_CARD_TEST = 2,
	FT_INT_TEST = 3,
	SINGLE_CHECK_TEST = 4,
    LED_OFF_TEST = 5,
    WEAR_MODE_TEST = 6
} TEST_MODE_t;

typedef struct { 
    int32_t phase1_data_avg;
    int32_t phase2_data_avg;
    int32_t phase3_data_avg;
    int32_t phase4_data_avg;
    int32_t gr_ps_dif;
    int32_t red_ps_dif;
    int32_t ir_ps_dif;
    int32_t gr_data_final;
    int32_t red_data_final;
    int32_t ir_data_final;
    int32_t gr_data_high;
    int32_t single_gr_led_judge;
} FT_RESULTS_t;

void hx3690l_factory_ft_leak_light_test_config(void);
void hx3690l_factory_ft_card_test_config(void);
void hx3690l_factory_ft_int_test_config(void);

bool hx3690l_factory_test_read(int32_t *phase_data);
FT_RESULTS_t hx3690l_factroy_test(TEST_MODE_t  test_mode);

void hx3690l_lab_test(uint32_t  test_mode);
void hx3690l_lab_test_read(void);


#endif // _HX3690L_FACTORY_TEST_H_




