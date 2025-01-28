# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_EXTERNAL_LIBC)
add_subdirectory(libc)
add_subdirectory(posix)
endif()
add_subdirectory(os)
add_subdirectory(util)
add_subdirectory_ifdef(CONFIG_OPENAMP              open-amp)
add_subdirectory_ifdef(CONFIG_RBUF_LIB	rbuf)
add_subdirectory(crc)