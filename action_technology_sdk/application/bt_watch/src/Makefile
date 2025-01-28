subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/include
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/tool/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/btmusic/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/btcall/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/ota/

ifdef CONFIG_OTA_RECOVERY_APP
obj-y += ota_recovery/
else
obj-y += main/

obj-$(CONFIG_BT_MUSIC_APP) += btmusic/
obj-$(CONFIG_BT_CALL_APP) += btcall/
obj-$(CONFIG_OTA_APP) += ota/
obj-$(CONFIG_TOOL) += tool/
obj-$(CONFIG_ACTIONS_ATT) +=att/

obj-$(CONFIG_LCMUSIC_APP) += lcmusic/
obj-$(CONFIG_LOCAL_PLAYER) += local_player/
obj-$(CONFIG_CARD_READER_APP) += card_reader/
obj-$(CONFIG_CHARGER_APP) += charger/
obj-$(CONFIG_BT_TRANSMIT) += bt_transmit/
#obj-$(CONFIG_AP_USER_COMM) += user_comm/
#obj-$(CONFIG_TUYA_APP) += tuya/
#obj-$(CONFIG_NV_APP) += nativevoice/
obj-$(CONFIG_NSM_APP) += bt_nsm/
obj-$(CONFIG_OTA_PRODUCT_SUPPORT) += ota_product/
obj-$(CONFIG_OTA_BLE_MASTER_SUPPORT) += ota_product/
endif
