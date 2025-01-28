
if(CONFIG_FAT_FILESYSTEM_ELM)
add_library(ELMFAT INTERFACE)

target_include_directories(ELMFAT INTERFACE include)

zephyr_library()
zephyr_library_sources(
    ff.c
    diskio.c
    )
zephyr_library_sources_ifdef(CONFIG_DISKIO_CACHE
    diskio_cache.c
)
zephyr_library_sources_ifdef(CONFIG_FAT_FILESYSTEM_ELM
    option/unicode.c
    option/syscall.c
)
add_subdirectory(nls)
zephyr_library_link_libraries(ELMFAT)
target_link_libraries(ELMFAT INTERFACE zephyr_interface)
endif()
