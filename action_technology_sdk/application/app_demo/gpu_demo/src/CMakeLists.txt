# SPDX-License-Identifier: Apache-2.0

target_include_directories(app PRIVATE
	.
	include
)

target_compile_options(app PRIVATE
	-Wno-char-subscripts
#	-Wno-unused-function
#	-Wno-unused-variable
)

add_subdirectory(main)
add_subdirectory(launcher)
add_subdirectory_ifdef(CONFIG_UI_MANAGER ui)
