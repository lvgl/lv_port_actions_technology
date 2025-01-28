#include "heap.h"

void * buddy_alloc(uint8_t buddy_no, int32_t *size)
{
    return rom_buddy_alloc(buddy_no, size, &g_rom_buddy_data);
}

int buddy_free(uint8_t buddy_no, void *where, unsigned long *info)
{
    return rom_buddy_free(buddy_no, where, info, &g_rom_buddy_data);
}
