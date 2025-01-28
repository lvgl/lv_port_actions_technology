# SPDX-License-Identifier: Apache-2.0

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE spng.c)
target_compile_definitions(app PRIVATE
	SPNG_DISABLE_OPT
	SPNG_NO_STDIO
	SPNG_USE_MINIZ
)
