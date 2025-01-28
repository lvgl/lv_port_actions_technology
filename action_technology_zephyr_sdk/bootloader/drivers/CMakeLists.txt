# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

add_subdirectory_ifdef(CONFIG_CONSOLE console)
add_subdirectory_ifdef(CONFIG_UART_ACTS         serial)
add_subdirectory_ifdef(CONFIG_I2C               i2c)
add_subdirectory_ifdef(CONFIG_GPIO_ACTS         gpio)
add_subdirectory_ifdef(CONFIG_DMA_ACTS          dma)
add_subdirectory_ifdef(CONFIG_SPI               spi)
add_subdirectory_ifdef(CONFIG_SPI_FLASH_ACTS    flash)

add_subdirectory_ifdef(CONFIG_DISPLAY       display)
add_subdirectory_ifdef(CONFIG_IPMSG_ACTS        ipmsg)
add_subdirectory(misc)
add_subdirectory_ifdef(CONFIG_MMC               mmc)
add_subdirectory_ifdef(CONFIG_BT_DRV            bluetooth)
add_subdirectory_ifdef(CONFIG_NVRAM_CONFIG      nvram)

add_subdirectory_ifdef(CONFIG_DSP      			dsp)
add_subdirectory_ifdef(CONFIG_ADC      			adc)
add_subdirectory_ifdef(CONFIG_INPUT_DEV      	input)

add_subdirectory_ifdef(CONFIG_WATCHDOG          watchdog)
add_subdirectory_ifdef(CONFIG_SPINAND_ACTS    	spinand)
add_subdirectory(timer)

add_subdirectory_ifdef(CONFIG_PWM          pwm)

add_subdirectory_ifdef(CONFIG_MPU_ACTS          mpu)
