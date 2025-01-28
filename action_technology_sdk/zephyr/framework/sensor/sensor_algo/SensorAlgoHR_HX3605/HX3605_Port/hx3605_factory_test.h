#ifndef _hx3605_FACTORY_TEST_H_
#define _hx3605_FACTORY_TEST_H_

#include <stdint.h>
#include <stdbool.h>

#define FT_OPEN_G_LED_LTH     3000 
#define FT_OPEN_R_LED_LTH     3000
#define FT_OPEN_IR_LED_LTH    3000

#define FT_CARD_G_LED_HTH     200000 
#define FT_CARD_R_LED_HTH     400000
#define FT_CARD_IR_LED_HTH    400000

typedef enum {
	LEAK_LIGHT_TEST = 1,
	GRAY_CARD_TEST = 2,
	FT_INT_TEST = 3
} TEST_MODE_t;


//void hx3605_factory_test_config(void);
void hx3605_factory_ft_leak_light_test_config(int8_t mode);
void hx3605_factory_ft_card_test_config(int8_t mode);
void hx3605_factory_ft_int_test_config(int8_t mode);

bool hx3605_factory_test_read(int32_t *phase1_data,int32_t *phase2_data,int32_t *phase3_data,int32_t *phase4_data);
bool hx3605_factroy_test(uint32_t  test_mode);

void hx3605_lab_test(uint32_t  test_mode);
void hx3605_lab_test_read(void);


#endif // _hx3605_FACTORY_TEST_H_




