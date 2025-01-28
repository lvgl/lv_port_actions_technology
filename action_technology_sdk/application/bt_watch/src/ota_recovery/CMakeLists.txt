# SPDX-License-Identifier: Apache-2.0

target_sources_ifdef(CONFIG_OTA_RECOVERY_APP app PRIVATE recovery_main.c)

