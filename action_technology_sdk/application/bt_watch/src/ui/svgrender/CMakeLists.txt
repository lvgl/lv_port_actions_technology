# SPDX-License-Identifier: Apache-2.0

FILE(GLOB SOURCES *.c)

target_sources(app PRIVATE ${SOURCES})
add_subdirectory(libspng-0.7.4)
add_subdirectory(miniz-3.0.2)
