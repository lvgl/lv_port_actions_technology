# SPDX-License-Identifier: Apache-2.0

target_sources_ifdef(CONFIG_NSM_APP app PRIVATE nsm_test_background.c)
target_sources_ifdef(CONFIG_NSM_APP app PRIVATE nsm_protocol.c)
target_sources_ifdef(CONFIG_NSM_APP app PRIVATE nsm_app_main.c)
target_sources_ifdef(CONFIG_NSM_APP app PRIVATE nsm_test_backend.c)
target_sources_ifdef(CONFIG_NSM_APP app PRIVATE nsm_interface.c)