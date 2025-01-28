#ifndef __PORTABLE_INCLUDE_AIC_GPIO_WAKEUP_H__
#define __PORTABLE_INCLUDE_AIC_GPIO_WAKEUP_H__

/* 归芯SDK已经实现的接口 */
/*
*********************************************************************
* Description: aic_gpio_mcu_can_sleep MCU可以自由睡眠
*              当gpio睡眠信号来时调用
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_mcu_can_sleep(void);

/*
*********************************************************************
* Description: aic_gpio_mcu_keep_wakeup MCU保持唤醒
*              当gpio唤醒信号来时调用
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_mcu_keep_wakeup(void);

/*
*********************************************************************
* Description: aic_gpio_get_modem_wakeup_status 获取modem的唤醒状态
* Arguments  : None.
* Return     : 0 睡眠
*              1 唤醒
* Note(s)    : None.
*********************************************************************
*/
u32 aic_gpio_get_modem_wakeup_status(void);

/* 需要第三方mcu实现的依赖接口 */
/*
*********************************************************************
* Description: aic_gpio_get_request_wakelock
*             为request获取一个不让mcu睡眠的锁
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_get_request_wakelock(void);
/*
*********************************************************************
* Description: aic_gpio_put_request_wakelock
*              释放request的唤醒锁
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_put_request_wakelock(void);

/*
*********************************************************************
* Description: aic_gpio_get_ack_wakelock
*              为ACK获取一个不让mcu睡眠的锁
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_get_ack_wakelock(void);
/*
*********************************************************************
* Description: aic_gpio_put_ack_wakelock
*              释放ACK的唤醒锁
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_put_ack_wakelock(void);

/*
*********************************************************************
* Description: aic_gpio_wait_modem_wakeup
*              等待modem唤醒
* Arguments  : timeout 等待多少ms
* Return     : 0  success
*              -1 fail
* Note(s)    : None.
*********************************************************************
*/
int aic_gpio_wait_modem_wakeup(u32 timeout);

#endif /* __PORTABLE_INCLUDE_AIC_GPIO_WAKEUP_H__ */
