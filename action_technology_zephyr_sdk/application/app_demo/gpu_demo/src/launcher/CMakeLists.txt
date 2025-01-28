# SPDX-License-Identifier: Apache-2.0

file(GLOB_RECURSE SOURCES *.c)

target_sources(app PRIVATE ${SOURCES})
