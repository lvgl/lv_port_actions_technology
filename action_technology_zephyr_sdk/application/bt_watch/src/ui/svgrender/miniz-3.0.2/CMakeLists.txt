# SPDX-License-Identifier: Apache-2.0

file(GLOB sources *.c *.S)

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${sources})
target_compile_definitions(app PRIVATE
	MINIZ_NO_MALLOC
	MINIZ_NO_STDIO
	MINIZ_NO_TIME
)
