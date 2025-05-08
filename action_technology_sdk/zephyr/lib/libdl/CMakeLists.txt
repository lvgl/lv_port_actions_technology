# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(
	.
	inc
)

zephyr_library_sources(
	arch/arm.c
	dlclose.c
	dlelf.c
	dlerror.c
	dlmodule.c
	dlopen.c
	dlsym.c
	dlsyms.c
	dlheap.c
)
