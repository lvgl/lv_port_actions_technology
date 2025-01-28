# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PANEL_ER76288A panel_device.c panel_er76288a.c)
zephyr_sources_ifdef(CONFIG_PANEL_FT2308 panel_device.c panel_ft2308.c)
zephyr_sources_ifdef(CONFIG_PANEL_GC9C01 panel_device.c panel_gc9c01.c)
zephyr_sources_ifdef(CONFIG_PANEL_ICNA3310B panel_device.c panel_icna3310b.c)
zephyr_sources_ifdef(CONFIG_PANEL_ICNA3311 panel_device.c panel_icna3311.c)
zephyr_sources_ifdef(CONFIG_PANEL_RM69090 panel_device.c panel_rm69090.c)
zephyr_sources_ifdef(CONFIG_PANEL_RM690B0 panel_device.c panel_rm690b0.c)
zephyr_sources_ifdef(CONFIG_PANEL_SH8601Z0 panel_device.c panel_sh8601z0.c)
zephyr_sources_ifdef(CONFIG_PANEL_ST77916 panel_device.c panel_st77916.c)
zephyr_sources_ifdef(CONFIG_PANEL_ST7802 panel_device.c panel_st7802.c)
zephyr_sources_ifdef(CONFIG_PANEL_JD9853 panel_device.c panel_jd9853.c)

# TR panels
zephyr_sources_ifdef(CONFIG_PANEL_LPM015M135A panel_device.c panel_lpm015m135a.c)

# RAM-less panels
zephyr_sources_ifdef(CONFIG_PANEL_AXS15231B panel_device_ramless.c panel_axs15231b.c)
zephyr_sources_ifdef(CONFIG_PANEL_ST77903 panel_device_ramless.c panel_st77903.c)
