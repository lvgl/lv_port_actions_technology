# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BT_DRV bt_drv.c)
zephyr_library_sources_ifdef(CONFIG_BT_HCI_ACTS hci_acts.c)
zephyr_library_sources_ifdef(CONFIG_BT_ECC_ACTS ecc_acts.c)
zephyr_library_sources_ifdef(CONFIG_BT_CTRL_BQB bqb_acts.c)
