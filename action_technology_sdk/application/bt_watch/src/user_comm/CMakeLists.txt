# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE aes_mw/aes_mw.c)
target_sources(app PRIVATE ap_record/record_app_event.c)
target_sources(app PRIVATE ap_record/record_app_main.c)
target_sources(app PRIVATE ap_record/record_app_media.c)
target_sources(app PRIVATE ap_record/record_app_uploadstream.c)
target_sources(app PRIVATE ap_status/ap_status.c)
target_sources(app PRIVATE sys_comm/sys_comm.c)
target_sources(app PRIVATE ota_transmit/ap_ota_stream.c)
target_sources(app PRIVATE ota_transmit/ap_ota_backend.c)
target_sources(app PRIVATE ota_transmit/ota_mw.c)
target_sources(app PRIVATE ota_transmit/ap_ota_transmit.c)
target_sources(app PRIVATE ota_transmit/ap_ota_head_splice.c)