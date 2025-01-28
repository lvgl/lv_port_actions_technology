zephyr_include_directories(include)

zephyr_library_sources(
    page.c
	page_init.c
	rom_page.c
	buddy.c
	buddy_dump.c
	buddy_init.c
	rom_buddy.c
	dump.c
	malloc.c
	free.c
)