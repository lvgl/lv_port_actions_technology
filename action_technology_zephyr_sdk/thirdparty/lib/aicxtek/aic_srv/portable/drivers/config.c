#include "aic_cfg.h"

/*
 * The aic uart config table.
 */
static aic_uart_cfg_t s_aic_uart_cfg = {
    .tx_pin = 63,
    .rx_pin = 62,
    .baud = 4000000,
};

/*
 * The aic gpio wakeup config table.
 */
static aic_gpio_wakeup_cfg_t s_aic_gpio_wakeup_cfg =
{
    .mcu_wakeup_modem = {.gpio_num = 57, .is_active_low = 1},
    .modem_wakeup_mcu = {.gpio_num = 58, .is_active_low = 1},
};

aic_uart_cfg_t *aic_get_uart_cfg(void)
{
    return &s_aic_uart_cfg;
}

aic_gpio_wakeup_cfg_t *aic_gpio_wakeup_cfg(void)
{
    return &s_aic_gpio_wakeup_cfg;
}
