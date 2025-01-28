# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE nv_analyze/nv_protocol.c)
target_sources(app PRIVATE nv_sppble/nv_ble_stream.c)
target_sources(app PRIVATE nv_sppble/nv_ble_stream_ctrl.c)
target_sources(app PRIVATE nv_analyze/library/aes.c)
target_sources(app PRIVATE nv_analyze/library/cipher.c)
target_sources(app PRIVATE nv_analyze/library/cipher_wrap.c)
target_sources(app PRIVATE nv_analyze/library/gcm.c)
target_sources(app PRIVATE nv_analyze/library/platform_util.c)