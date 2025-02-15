# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)
zephyr_include_directories(m_effect)

target_sources(app PRIVATE launcher_app.c)

if (CONFIG_UI_MANAGER)

FILE(GLOB applist_src applist/*.c)
FILE(GLOB health_src health/*.c)
FILE(GLOB longlist_src longlist/*.c)
FILE(GLOB power_src power/*.c)
FILE(GLOB stopwatch_src stopwatch/*.c)
FILE(GLOB three_dimensional_src three_dimensional/*.c)
FILE(GLOB m_effect_src m_effect/*.c)
FILE(GLOB ui_coder_src ui_coder/*.c)
FILE(GLOB setting_src setting/*.c)

target_sources(app PRIVATE
	clock_view.c
	digital1_clock_view.c
	video_clock_view.c
	gif_view.c
	heart_view.c
	main_view.c
	message_view.c
	music_view.c
	sport_view.c
	aod_clock_view.c
	alipay_main_view.c
	wxpay_main_view.c
	launcher_presenter.c
	${applist_src}
	${health_src}
	${longlist_src}
	${power_src}
	${stopwatch_src}
	${three_dimensional_src}
	${m_effect_src}
	${ui_coder_src}
	${setting_src}
)

if(CONFIG_ALARM_MANAGER)
	target_sources(app PRIVATE alarm_set_view.c)
endif()

add_subdirectory(clock_selector)
add_subdirectory(alipay)
add_subdirectory(gps)
add_subdirectory(vibration)

if(CONFIG_CAT1_APP)
	add_subdirectory(sim)
endif()

if(CONFIG_THIRD_PARTY_APP)
add_subdirectory(third_party_app)
endif()

if(CONFIG_AWK_LIB)
add_subdirectory(awk_map)
endif()

endif()
