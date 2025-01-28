# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(i2c_common.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SHELL		i2c_shell.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ACTS		i2c_acts.c)
zephyr_library_sources_ifdef(CONFIG_I2CMT_LARK		i2cmt_lark.c)
