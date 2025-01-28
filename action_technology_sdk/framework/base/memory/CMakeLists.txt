zephyr_include_directories(.)

zephyr_library_sources(
    mem_manager.c
)

zephyr_library_sources_ifdef(CONFIG_APP_USED_MEM_POOL
    mem_pool.c
)

zephyr_library_sources_ifdef(CONFIG_APP_USED_MEM_SLAB
    mem_slab.c
)

zephyr_library_sources_ifdef(CONFIG_APP_USED_MEM_PAGE
    mem_page.c
)

zephyr_library_sources_ifdef(CONFIG_MEM_GUARD
    mem_guard.c
)

add_subdirectory_ifdef(CONFIG_APP_USED_MEM_PAGE page_buddy)

