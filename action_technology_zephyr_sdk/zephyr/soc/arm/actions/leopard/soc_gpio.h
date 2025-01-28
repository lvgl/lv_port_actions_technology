/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file GPIO/PINMUX configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_GPIO_H_
#define	_ACTIONS_SOC_GPIO_H_
#include <device.h>


/** @brief This enum defines the normal GPIO port.
 *  The platform supports a total of 78 GPIO pins with various functionality.
 *
*/
typedef enum {
    GPIO_0 = 0,                            /**< GPIO pin0. */
    GPIO_1 = 1,                            /**< GPIO pin1. */
    GPIO_2 = 2,                            /**< GPIO pin2. */
    GPIO_3 = 3,                            /**< GPIO pin3. */
    GPIO_4 = 4,                            /**< GPIO pin4. */
    GPIO_5 = 5,                            /**< GPIO pin5. */
    GPIO_6 = 6,                            /**< GPIO pin6. */
    GPIO_7 = 7,                            /**< GPIO pin7. */
    GPIO_8 = 8,                            /**< GPIO pin8. */
    GPIO_9 = 9,                            /**< GPIO pin9. */
    GPIO_10 = 10,                          /**< GPIO pin10. */
    GPIO_11 = 11,                          /**< GPIO pin11. */
    GPIO_12 = 12,                          /**< GPIO pin12. */
    GPIO_13 = 13,                          /**< GPIO pin13. */
    GPIO_14 = 14,                          /**< GPIO pin14. */
    GPIO_15 = 15,                          /**< GPIO pin15. */
    GPIO_16 = 16,                          /**< GPIO pin16. */
    GPIO_17 = 17,                          /**< GPIO pin17. */
    GPIO_18 = 18,                          /**< GPIO pin18. */
    GPIO_19 = 19,                          /**< GPIO pin19. */
    GPIO_20 = 20,                          /**< GPIO pin20. */
    GPIO_21 = 21,                          /**< GPIO pin21. */
    GPIO_22 = 22,                          /**< GPIO pin22. */
    GPIO_23 = 23,                          /**< GPIO pin23. */
    GPIO_24 = 24,                          /**< GPIO pin24. */
    GPIO_25 = 25,                          /**< GPIO pin25. */
    GPIO_26 = 26,                          /**< GPIO pin26. */
    GPIO_27 = 27,                          /**< GPIO pin27. */
    GPIO_28 = 28,                          /**< GPIO pin28. */
    GPIO_29 = 29,                          /**< GPIO pin29. */
    GPIO_30 = 30,                          /**< GPIO pin30. */
    GPIO_31 = 31,                          /**< GPIO pin31. */
    GPIO_32 = 32,                          /**< GPIO pin32. */
    GPIO_33 = 33,                          /**< GPIO pin33. */
    GPIO_34 = 34,                          /**< GPIO pin34. */
    GPIO_35 = 35,                          /**< GPIO pin35. */
    GPIO_36 = 36,                          /**< GPIO pin36. */
    GPIO_37 = 37,                          /**< GPIO pin37. */
    GPIO_38 = 38,                          /**< GPIO pin38. */
    GPIO_39 = 39,                          /**< GPIO pin39. */
    GPIO_40 = 40,                          /**< GPIO pin40. */
    GPIO_41 = 41,                          /**< GPIO pin41. */
    GPIO_42 = 42,                          /**< GPIO pin42. */
    GPIO_43 = 43,                          /**< GPIO pin43. */
    GPIO_44 = 44,                          /**< GPIO pin44. */
    GPIO_45 = 45,                          /**< GPIO pin45. */
    GPIO_46 = 46,                          /**< GPIO pin46. */
    GPIO_47 = 47,                          /**< GPIO pin47. */
    GPIO_48 = 48,                          /**< GPIO pin48. */
    GPIO_49 = 49,                          /**< GPIO pin49. */
    GPIO_50 = 50,                          /**< GPIO pin50. */
    GPIO_51 = 51,                          /**< GPIO pin51. */
    GPIO_52 = 52,                          /**< GPIO pin52. */
    GPIO_53 = 53,                          /**< GPIO pin53. */
    GPIO_54 = 54,                          /**< GPIO pin54. */
    GPIO_55 = 55,                          /**< GPIO pin55. */
    GPIO_56 = 56,                          /**< GPIO pin56. */
    GPIO_57 = 57,                          /**< GPIO pin57. */
    GPIO_58 = 58,                          /**< GPIO pin58. */
    GPIO_59 = 59,                          /**< GPIO pin59. */
    GPIO_60 = 60,                          /**< GPIO pin60. */
    GPIO_61 = 61,                          /**< GPIO pin61. */
    GPIO_62 = 62,                          /**< GPIO pin62. */
    GPIO_63 = 63,                          /**< GPIO pin63. */
    GPIO_64 = 64,                          /**< GPIO pin64. */
    GPIO_65 = 65,                          /**< GPIO pin65. */
    GPIO_66 = 66,                          /**< GPIO pin66. */
    GPIO_67 = 67,                          /**< GPIO pin67. */
    GPIO_68 = 68,                          /**< GPIO pin68. */
    GPIO_69 = 69,                          /**< GPIO pin69. */
    GPIO_70 = 70,                          /**< GPIO pin70. */
    GPIO_71 = 71,                          /**< GPIO pin71. */
    GPIO_72 = 72,                          /**< GPIO pin72. */
    GPIO_73 = 73,                          /**< GPIO pin73. */
    GPIO_74 = 74,                          /**< GPIO pin74. */
    GPIO_75 = 75,                          /**< GPIO pin75. */
    GPIO_76 = 76,                          /**< GPIO pin76. */
    GPIO_77 = 77,                          /**< GPIO pin77. */
    GPIO_78 = 78,                          /**< GPIO pin78. */
    GPIO_79 = 79,                          /**< GPIO pin79. */
    GPIO_80 = 80,                          /**< GPIO pin80. */
    GPIO_MAX_PIN_NUM                       /**< The total number of GPIO pins (invalid GPIO pin number). */
} gpio_pin_e;

/** @brief This enum defines the WIO port.
 *  The platform supports a total of 4 WIO pins with various functionality.
 *
*/
typedef enum {
    WIO_0 = 0,                            /**< WIO pin0. */
    WIO_1 = 1,                            /**< WIO pin1. */
    WIO_2 = 2,                            /**< WIO pin2. */
    WIO_3 = 3,                            /**< WIO pin3. */
    GPIO_WIO_MAX_PIN_NUM                  /**< The total number of WIO pins (invalid WIO pin number). */
} wio_pin_e;

#define GPIO_MAX_GROUPS				(((GPIO_MAX_PIN_NUM) + 31) / 32)
#define GPIO_MAX_IRQ_GRP			3
#define GPIO_MAX_INT_PIN_NUM        64

#define GPIO_WIO_REG_BASE			(GPIO_REG_BASE + 0x300)

#define GPIO_CTL0				0x0
#define GPIO_ODAT0				0x200
#define GPIO_BSR0				0x210
#define GPIO_BRR0				0x220
#define GPIO_IDAT0				0x230
#define GPIO_IRQ_PD0			0x240

#define JTAG_CTL				0x40068400
#define SC_SWEN_SHIFT			12
#define SC_SWMAP_SHIFT			8
#define JTAG_CTL_SC_SWEN(x)		((x) << SC_SWEN_SHIFT)
#define JTAG_CTL_SC_SWMAP(x)	((x) << SC_SWMAP_SHIFT)

#define GPIO_REG_CTL(base, pin)			((base) + GPIO_CTL0 + (pin) * 4)
#define GPIO_REG_ODAT(base, pin)		((base) + GPIO_ODAT0 + (pin) / 32 * 4)
#define GPIO_REG_IDAT(base, pin)		((base) + GPIO_IDAT0 + (pin) / 32 * 4)
#define GPIO_REG_BSR(base, pin)			((base) + GPIO_BSR0 + (pin) / 32 * 4)
#define GPIO_REG_BRR(base, pin)			((base) + GPIO_BRR0 + (pin) / 32 * 4)
#define GPIO_REG_IRQ_PD(base, pin)		((base) + GPIO_IRQ_PD0 + (pin) / 32 * 4)
#define GPIO_BIT(pin)					(1 << ((pin) % 32))

#define GPION_CTL(n)					GPIO_REG_CTL(GPIO_REG_BASE,n)
#define GPION_ODAT(n)					GPIO_REG_ODAT(GPIO_REG_BASE,n)
#define GPION_IDAT(n)					GPIO_REG_IDAT(GPIO_REG_BASE,n)
#define GPION_BSR(n)					GPIO_REG_BSR(GPIO_REG_BASE,n)
#define GPION_BRR(n)					GPIO_REG_BRR(GPIO_REG_BASE,n)
#define GPION_IRQ_PD(n)					GPIO_REG_IRQ_PD(GPIO_REG_BASE,n)

#define WIO_REG_CTL(pin)				(GPIO_WIO_REG_BASE + (pin) * 4)
#define WIO_CTL_INT_PD_SHIFT            (24)
#define WIO_CTL_INT_PD_MASK             (1 << WIO_CTL_INT_PD_SHIFT)


#define GPIO_CTL_MFP_SHIFT				(0)
#define GPIO_CTL_MFP_MASK				(0x1f << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP_GPIO				(0x0 << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_SMIT					(0x1 << 5)
#define GPIO_CTL_GPIO_OUTEN				(0x1 << 6)
#define GPIO_CTL_GPIO_INEN				(0x1 << 7)
#define GPIO_CTL_PULL_MASK				(0xf << 8)
#define GPIO_CTL_PULLUP_STRONG			(0x1 << 8)
#define GPIO_CTL_PULLDOWN				(0x1 << 9)
#define GPIO_CTL_PULLUP					(0x1 << 11)
#define GPIO_CTL_PADDRV_SHIFT			(12)
#define GPIO_CTL_PADDRV_LEVEL(x)		((x) << GPIO_CTL_PADDRV_SHIFT)
#define GPIO_CTL_PADDRV_MASK			GPIO_CTL_PADDRV_LEVEL(0x7)
#define GPIO_CTL_INTC_EN				(0x1 << 20)
#define GPIO_CTL_INC_TRIGGER_SHIFT			(21)
#define GPIO_CTL_INC_TRIGGER(x)				((x) << GPIO_CTL_INC_TRIGGER_SHIFT)
#define GPIO_CTL_INC_TRIGGER_MASK			GPIO_CTL_INC_TRIGGER(0x7)
#define GPIO_CTL_INC_TRIGGER_RISING_EDGE	GPIO_CTL_INC_TRIGGER(0x0)
#define GPIO_CTL_INC_TRIGGER_FALLING_EDGE	GPIO_CTL_INC_TRIGGER(0x1)
#define GPIO_CTL_INC_TRIGGER_DUAL_EDGE		GPIO_CTL_INC_TRIGGER(0x2)
#define GPIO_CTL_INC_TRIGGER_HIGH_LEVEL		GPIO_CTL_INC_TRIGGER(0x3)
#define GPIO_CTL_INC_TRIGGER_LOW_LEVEL		GPIO_CTL_INC_TRIGGER(0x4)
#define GPIO_CTL_INTC_MASK					(0x1 << 25)

#define PINMUX_MODE_MASK			(GPIO_CTL_MFP_MASK | \
                         GPIO_CTL_PULL_MASK | GPIO_CTL_PADDRV_MASK | \
                         GPIO_CTL_SMIT)


static inline void soc_gpio_output(uint32_t pin, uint32_t val)
{
	/* set gpio value */
	if (val) {
		sys_write32(GPIO_BIT(pin), GPION_BSR(pin));
	} else {
		sys_write32(GPIO_BIT(pin), GPION_BRR(pin));
	}
	/* config gpio output */
	sys_write32(0x3840, GPION_CTL(pin));
}

#endif /* _ACTIONS_SOC_GPIO_H_	*/
