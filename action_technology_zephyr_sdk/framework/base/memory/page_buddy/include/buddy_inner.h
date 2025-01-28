#ifndef __BUDDY_INNER_H
#define __BUDDY_INNER_H

#define CONFIG_POOL_PAGE_SHIFT 11

/*MAX_INDEX default value is 128 if this changed must modify modle */
#define MAX_INDEX   128
#define UNIT_SIZE   (PAGE_SIZE / MAX_INDEX)
#define SELF_SIZE   128
#define SELF_INDEX  (MAX_INDEX - (SELF_SIZE / UNIT_SIZE))
#define MAX_FREE_SIZE   (PAGE_SIZE - SELF_SIZE)

#define LEFT_LEAF(index)    ((index) * 2 + 1)
#define RIGHT_LEAF(index)   ((index) * 2 + 2)
#define PARENT(index)       (((index) + 1) / 2 - 1)
#define RIGHT_SIBLING_LEFT_LEAF(index)  ((index) * 2 + 3)

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))

/*
 * Takes a pointer and yields a truncated value to be saved into `struct
 * log_item::ptr`
 *
 * See `PTR_INFLATE()` 
 * used 26 bits for address ring no need 32bits
 */
#define PTR_DEFLATE(ptr) \
  ((ptr) ? ((unsigned int) (ptr) & 0x03ffffff) : 0)

#define PTR_INFLATE(v) \
      (((v) != 0) ? ((void *) (0x00000000 | ((v) & 0x03ffffff))) : NULL)

#define INVALID_THREAD_PRIO     (-16)

struct buddy {
    uint8_t page_no;
    uint8_t max[71];
    uint32_t type[8];
    uint8_t freed;
    uint8_t reserved[23];
};

typedef struct
{
    int (*printf)( const char *fmt, ...);
    void*(*pagepool_convert_index_to_addr)(unsigned char index);
    void (*halt)(void);
}rom_buddy_data_t;

extern rom_buddy_data_t g_rom_buddy_data;


#define BUDDY_SELF_INDEX (30)
#define BUDDY_SELF_SIZE (SELF_SIZE / UNIT_SIZE)

uint8_t getMax(struct buddy* self, int index);
void setMax(struct buddy* self, int index, uint8_t val);

extern int rom_buddy_getType(struct buddy* self, int index);
extern void * rom_buddy_alloc(uint8_t buddy_no, int32_t *size, rom_buddy_data_t *ctx);
extern int rom_buddy_free(uint8_t buddy_no, void *where, unsigned long *info, rom_buddy_data_t *ctx);

uint8_t rom_new_buddy_no(uint8_t page_no, rom_buddy_data_t *ctx);
uint8_t new_buddy_no(uint8_t page_no);
void * buddy_alloc(uint8_t buddy_no, int32_t *size);
int buddy_free(uint8_t buddy_no, void *where, unsigned long *info);
bool buddy_dump(uint8_t buddy_no, unsigned int print_detail,const char *match_str);
int get_buddy_max(void);
bool is_buddy_idled(uint8_t buddy_no);

bool rom_is_buddy_idled(uint8_t buddy_no, rom_buddy_data_t *ctx);
void buddy_rom_data_init(void);

void dump_show_info(void *addr, int size, unsigned long *p_info, unsigned int print_detail, const char *match_str);
#endif
