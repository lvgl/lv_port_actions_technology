/*
 * from linux/kernel/kallsyms.c
 *
 * kallsyms.c: in-kernel printing of symbolic oopses and stack traces.
 *
 * Rewritten and vastly simplified by Rusty Russell for in-kernel
 * module loader:
 *   Copyright 2002 Rusty Russell <rusty@rustcorp.com.au> IBM Corporation
 *
 * ChangeLog:
 *
 * (25/Aug/2004) Paulo Marques <pmarques@grupopie.com>
 *      Changed the compression method from stem compression to "table lookup"
 *      compression (see scripts/kallsyms.c for a more complete description)
 */

#include <kernel.h>
#include <string.h>
#include <stdio.h>
#include <kallsyms.h>
#include <sdfs.h>


#if 0
/*
 * These will be re-linked against their real values
 * during the second link stage.
 */
extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const u8_t kallsyms_names[] __attribute__((weak));

/*
 * Tell the compiler that the count isn't in the small data section if the arch
 * has one (eg: FRV).
 */
extern const unsigned long kallsyms_num_syms
__attribute__((weak, section(".rodata")));

extern const u8_t kallsyms_token_table[] __attribute__((weak));
extern const u16_t kallsyms_token_index[] __attribute__((weak));

extern const unsigned long kallsyms_markers[] __attribute__((weak));

extern const unsigned long __text_region_start, __text_region_end;
extern const unsigned long __ramfunc_start, __ramfunc_end;

int is_ksym_addr(unsigned long addr)
{
    if ((addr >= (unsigned long)&__text_region_start &&
         addr <= (unsigned long)&__text_region_end))
        return 1;

    if ((addr >= (unsigned long)&__ramfunc_start &&
         addr <= (unsigned long)&__ramfunc_end))
        return 1;

    return 0;
}

/*
 * Expand a compressed symbol data into the resulting uncompressed string,
 * if uncompressed string is too long (>= maxlen), it will be truncated,
 * given the offset to where the symbol is in the compressed stream.
 */
static unsigned int kallsyms_expand_symbol(unsigned int off,
                       char *result, size_t maxlen)
{
    int len, skipped_first = 0;
    const u8_t *tptr, *data;

    /* Get the compressed symbol length from the first symbol byte. */
    data = &kallsyms_names[off];
    len = *data;
    data++;

    /*
     * Update the offset to return the offset for the next symbol on
     * the compressed stream.
     */
    off += len + 1;

    /*
     * For every byte on the compressed symbol data, copy the table
     * entry for that byte.
     */
    while (len) {
        tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
        data++;
        len--;

        while (*tptr) {
            if (skipped_first) {
                if (maxlen <= 1)
                    goto tail;
                *result = *tptr;
                result++;
                maxlen--;
            } else
                skipped_first = 1;
            tptr++;
        }
    }

tail:
    if (maxlen)
        *result = '\0';

    /* Return to offset to the next symbol. */
    return off;
}

/*
 * Find the offset on the compressed stream given and index in the
 * kallsyms array.
 */
static unsigned int get_symbol_offset(unsigned long pos)
{
    const u8_t *name;
    int i;

    /*
     * Use the closest marker we have. We have markers every 256 positions,
     * so that should be close enough.
     */
    name = &kallsyms_names[kallsyms_markers[pos >> 8]];

    /*
     * Sequentially scan all the symbols up to the point we're searching
     * for. Every symbol is stored in a [<len>][<len> bytes of data] format,
     * so we just need to add the len to the current pointer for every
     * symbol we wish to skip.
     */
    for (i = 0; i < (pos & 0xFF); i++)
        name = name + (*name) + 1;

    return name - kallsyms_names;
}

static unsigned long get_symbol_pos(unsigned long addr,
                    unsigned long *symbolsize,
                    unsigned long *offset)
{
    unsigned long symbol_start = 0, symbol_end = 0;
    unsigned long i, low, high, mid;

    /* Do a binary search on the sorted kallsyms_addresses array. */
    low = 0;
    high = kallsyms_num_syms;

    while (high - low > 1) {
        mid = low + (high - low) / 2;
        if (kallsyms_addresses[mid] <= addr)
            low = mid;
        else
            high = mid;
    }

    /*
     * Search for the first aliased symbol. Aliased
     * symbols are symbols with the same address.
     */
    while (low && kallsyms_addresses[low-1] == kallsyms_addresses[low])
        --low;

    symbol_start = kallsyms_addresses[low];

    /* Search for next non-aliased symbol. */
    for (i = low + 1; i < kallsyms_num_syms; i++) {
        if (kallsyms_addresses[i] > symbol_start) {
            symbol_end = kallsyms_addresses[i];
            break;
        }
    }

    /* If we found no next symbol, we use the end of the section. */
    if (!symbol_end) {
        symbol_end = (unsigned long)__text_region_end;
    }

    if (symbolsize)
        *symbolsize = symbol_end - symbol_start;
    if (offset)
        *offset = addr - symbol_start;

    return low;
}

/*
 * Lookup an address but don't bother to find any names.
 */
int kallsyms_lookup_size_offset(unsigned long addr, unsigned long *symbolsize,
                unsigned long *offset)
{
    if (is_ksym_addr(addr))
        return !!get_symbol_pos(addr, symbolsize, offset);

    return 0;
}

/*
 * Lookup an address
 * - modname is set to NULL if it's in the kernel.
 * - We guarantee that the returned name is valid until we reschedule even if.
 *   It resides in a module.
 * - We also guarantee that modname will be valid until rescheduled.
 */
const char *kallsyms_lookup(unsigned long addr,
                unsigned long *symbolsize,
                unsigned long *offset,
                char **modname, char *namebuf)
{
    namebuf[KSYM_NAME_LEN - 1] = 0;
    namebuf[0] = 0;

    if (is_ksym_addr(addr)) {
        unsigned long pos;

        pos = get_symbol_pos(addr, symbolsize, offset);
        /* Grab name */
        kallsyms_expand_symbol(get_symbol_offset(pos),
                       namebuf, KSYM_NAME_LEN);
        if (modname)
            *modname = NULL;
        return namebuf;
    }

    return NULL;
}

/* Look up a kernel symbol and return it in a text buffer. */
static int __sprint_symbol(char *buffer, unsigned long address,
               int symbol_offset, int add_offset)
{
    char *modname;
    const char *name;
    unsigned long offset, size;
    int len;

    address += symbol_offset;
    name = kallsyms_lookup(address, &size, &offset, &modname, buffer);
    if (!name)
        return sprintf(buffer, "0x%x", (unsigned int)address);

    if (name != buffer)
        strcpy(buffer, name);
    len = strlen(buffer);
    offset -= symbol_offset;

    if (add_offset)
        len += sprintf(buffer + len, "+%#x/%#x", (unsigned int)offset, (unsigned int)size);

    if (modname)
        len += sprintf(buffer + len, " [%s]", modname);

    return len;
}

/**
 * sprint_symbol - Look up a kernel symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function looks up a kernel symbol with @address and stores its name,
 * offset, size and module name to @buffer if possible. If no symbol was found,
 * just saves its @address as is.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_symbol(char *buffer, unsigned long address)
{
    return __sprint_symbol(buffer, address, 0, 1);
}

/**
 * sprint_symbol_no_offset - Look up a kernel symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function looks up a kernel symbol with @address and stores its name
 * and module name to @buffer if possible. If no symbol was found, just saves
 * its @address as is.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_symbol_no_offset(char *buffer, unsigned long address)
{
    return __sprint_symbol(buffer, address, 0, 0);
}

/**
 * sprint_backtrace - Look up a backtrace symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function is for stack backtrace and does the same thing as
 * sprint_symbol() but with modified/decreased @address. If there is a
 * tail-call to the function marked "noreturn", gcc optimized out code after
 * the call so that the stack-saved return address could point outside of the
 * caller. This function ensures that kallsyms will find the original caller
 * by decreasing @address.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_backtrace(char *buffer, unsigned long address)
{
    return __sprint_symbol(buffer, address, -1, 1);
}

/* Look up a kernel symbol and print it to the kernel messages. */
static void __print_symbol(const char *fmt, unsigned long address)
{
    char buffer[KSYM_SYMBOL_LEN];

    sprint_symbol(buffer, address);

    printk(fmt, buffer);
}

void print_symbol(const char *fmt, unsigned long addr)
{
    __print_symbol(fmt, (unsigned long)
               __builtin_extract_return_addr((void *)addr));
}

#else

/*
 * These will be re-linked against their real values
 * during the second link stage.
 */
static unsigned long *kallsyms_addresses;
static u8_t *kallsyms_names;
static unsigned long kallsyms_num_syms;

static u8_t *kallsyms_token_table;
static u16_t *kallsyms_token_index;
static unsigned long *kallsyms_markers;

extern const unsigned long __text_region_start, __text_region_end;
extern const unsigned long __ramfunc_start, __ramfunc_end;


int is_ksym_addr(unsigned long addr)
{
    if ((addr >= (unsigned long)&__text_region_start &&
         addr <= (unsigned long)&__text_region_end))
        return 1;

    if ((addr >= (unsigned long)&__ramfunc_start &&
         addr <= (unsigned long)&__ramfunc_end))
        return 1;

    return 0;
}

/*
 * Expand a compressed symbol data into the resulting uncompressed string,
 * if uncompressed string is too long (>= maxlen), it will be truncated,
 * given the offset to where the symbol is in the compressed stream.
 */
static unsigned int kallsyms_expand_symbol(unsigned int off,
                       char *result, size_t maxlen)
{
    int len, skipped_first = 0;
    const u8_t *tptr, *data;

    /* Get the compressed symbol length from the first symbol byte. */
    data = &kallsyms_names[off];
    len = *data;
    data++;

    /*
     * Update the offset to return the offset for the next symbol on
     * the compressed stream.
     */
    off += len + 1;

    /*
     * For every byte on the compressed symbol data, copy the table
     * entry for that byte.
     */
    while (len) {
        tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
        data++;
        len--;

        while (*tptr) {
            if (skipped_first) {
                if (maxlen <= 1)
                    goto tail;
                *result = *tptr;
                result++;
                maxlen--;
            } else
                skipped_first = 1;
            tptr++;
        }
    }

tail:
    if (maxlen)
        *result = '\0';

    /* Return to offset to the next symbol. */
    return off;
}

/*
 * Find the offset on the compressed stream given and index in the
 * kallsyms array.
 */
static unsigned int get_symbol_offset(unsigned long pos)
{
    const u8_t *name;
    int i;

    /*
     * Use the closest marker we have. We have markers every 256 positions,
     * so that should be close enough.
     */
    name = &kallsyms_names[kallsyms_markers[pos >> 8]];

    /*
     * Sequentially scan all the symbols up to the point we're searching
     * for. Every symbol is stored in a [<len>][<len> bytes of data] format,
     * so we just need to add the len to the current pointer for every
     * symbol we wish to skip.
     */
    for (i = 0; i < (pos & 0xFF); i++)
        name = name + (*name) + 1;

    return name - kallsyms_names;
}

static unsigned long get_symbol_pos(unsigned long addr,
                    unsigned long *symbolsize,
                    unsigned long *offset)
{
    unsigned long symbol_start = 0, symbol_end = 0;
    unsigned long i, low, high, mid;

    /* Do a binary search on the sorted kallsyms_addresses array. */
    low = 0;
    high = kallsyms_num_syms;

    while (high - low > 1) {
        mid = low + (high - low) / 2;
        if (kallsyms_addresses[mid] <= addr)
            low = mid;
        else
            high = mid;
    }

    /*
     * Search for the first aliased symbol. Aliased
     * symbols are symbols with the same address.
     */
    while (low && kallsyms_addresses[low-1] == kallsyms_addresses[low])
        --low;

    symbol_start = kallsyms_addresses[low];

    /* Search for next non-aliased symbol. */
    for (i = low + 1; i < kallsyms_num_syms; i++) {
        if (kallsyms_addresses[i] > symbol_start) {
            symbol_end = kallsyms_addresses[i];
            break;
        }
    }

    /* If we found no next symbol, we use the end of the section. */
    if (!symbol_end) {
        symbol_end = (unsigned long)__text_region_end;
    }

    if (symbolsize)
        *symbolsize = symbol_end - symbol_start;
    if (offset)
        *offset = addr - symbol_start;

    return low;
}

/*
 * Lookup an address but don't bother to find any names.
 */
int kallsyms_lookup_size_offset(unsigned long addr, unsigned long *symbolsize,
                unsigned long *offset)
{
    if (is_ksym_addr(addr))
        return !!get_symbol_pos(addr, symbolsize, offset);

    return 0;
}

/*
 * Lookup an address
 * - modname is set to NULL if it's in the kernel.
 * - We guarantee that the returned name is valid until we reschedule even if.
 *   It resides in a module.
 * - We also guarantee that modname will be valid until rescheduled.
 */
const char *kallsyms_lookup(unsigned long addr,
                unsigned long *symbolsize,
                unsigned long *offset,
                char **modname, char *namebuf)
{
    namebuf[KSYM_NAME_LEN - 1] = 0;
    namebuf[0] = 0;

    if (is_ksym_addr(addr)) {
        unsigned long pos;

        pos = get_symbol_pos(addr, symbolsize, offset);
        /* Grab name */
        kallsyms_expand_symbol(get_symbol_offset(pos),
                       namebuf, KSYM_NAME_LEN);
        if (modname)
            *modname = NULL;
        return namebuf;
    }

    return NULL;
}

/* Look up a kernel symbol and return it in a text buffer. */
static int __sprint_symbol(char *buffer, unsigned long address,
               int symbol_offset, int add_offset)
{
    char *modname;
    const char *name;
    unsigned long offset, size;
    int len;
    if(kallsyms_addresses == NULL){
        buffer[0] = '@';
        buffer[1] = 0;
        return 2;
    }

    address += symbol_offset;
    name = kallsyms_lookup(address, &size, &offset, &modname, buffer);
    if (!name)
        return sprintf(buffer, "0x%x", (unsigned int)address);

    if (name != buffer)
        strcpy(buffer, name);
    len = strlen(buffer);
    offset -= symbol_offset;

    if (add_offset)
        len += sprintf(buffer + len, "+%#x/%#x", (unsigned int)offset, (unsigned int)size);

    if (modname)
        len += sprintf(buffer + len, " [%s]", modname);

    return len;
}

/**
 * sprint_symbol - Look up a kernel symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function looks up a kernel symbol with @address and stores its name,
 * offset, size and module name to @buffer if possible. If no symbol was found,
 * just saves its @address as is.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_symbol(char *buffer, unsigned long address)
{
    return __sprint_symbol(buffer, address, 0, 1);
}

/**
 * sprint_symbol_no_offset - Look up a kernel symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function looks up a kernel symbol with @address and stores its name
 * and module name to @buffer if possible. If no symbol was found, just saves
 * its @address as is.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_symbol_no_offset(char *buffer, unsigned long address)
{
    return __sprint_symbol(buffer, address, 0, 0);
}

/**
 * sprint_backtrace - Look up a backtrace symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function is for stack backtrace and does the same thing as
 * sprint_symbol() but with modified/decreased @address. If there is a
 * tail-call to the function marked "noreturn", gcc optimized out code after
 * the call so that the stack-saved return address could point outside of the
 * caller. This function ensures that kallsyms will find the original caller
 * by decreasing @address.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_backtrace(char *buffer, unsigned long address)
{
    return __sprint_symbol(buffer, address, -1, 1);
}

/* Look up a kernel symbol and print it to the kernel messages. */
static void __print_symbol(const char *fmt, unsigned long address)
{
    char buffer[KSYM_SYMBOL_LEN];

    sprint_symbol(buffer, address);

    printk(fmt, buffer);
}

void print_symbol(const char *fmt, unsigned long addr)
{
    __print_symbol(fmt, (unsigned long)
               __builtin_extract_return_addr((void *)addr));
}


//TLV
/*
0x0- 0x03  magic
0x04-0x07  len
0x08-0x0b   checksum
0x0c-0x10  resrve
0x10---len  tlv
*/
#define TLV_MAGIC				0x59355935
#define TYPE_KYMS_ADDR          0x01             //kallsyms_addresses
#define TYPE_KYMS_NUM           0x02             //kallsyms_num_syms
#define TYPE_KYMS_NAME          0x03             //kallsyms_names
#define TYPE_KYMS_MARKERS       0x04             //kallsyms_markers
#define TYPE_KYMS_TOKEN_TABEL   0x05             //kallsyms_token_table
#define TYPE_KYMS_TOKEN_INDEX   0x06             //kallsyms_token_index
#include <device.h>

#if 0
static unsigned long *kallsyms_addresses;
static u8_t *kallsyms_names;
static unsigned long kallsyms_num_syms;

static u8_t *kallsyms_token_table;
static u16_t *kallsyms_token_index;
static unsigned long kallsyms_markers;
#endif

static int print_symb_init(const struct device *dev)
{
    unsigned int *sym_addr;
    unsigned int len, off;

    if(sd_fmap("ksym.bin", (void**)&sym_addr, &len)){
        printk("ksym.bin not in sdfs\n");
        return 0;
    }
    printk("ksym=%p, len=0x%x\n", sym_addr, len);
    if(sym_addr[0] != TLV_MAGIC){
        printk("sym:magic=0x%x != 0x59355935\n", sym_addr[0]);
        return 0;
    }
    off = 0x10/4;
    printk("1.off=0x%x, type=%d, len=0x%x\n", off*4, sym_addr[off], sym_addr[off+1]);
    if(sym_addr[off] != TYPE_KYMS_ADDR){
        printk("KYMS_ADDR type=%d invalid\n", sym_addr[off]);
        return 0;
    }
    kallsyms_addresses = (long unsigned int *)&sym_addr[off+2];
    off += sym_addr[off+1]/4+2;

    printk("2. off=0x%x, type=%d, len=0x%x\n", off*4, sym_addr[off], sym_addr[off+1]);
    if(sym_addr[off] != TYPE_KYMS_NUM){
        printk("TYPE_KYMS_NUM type=%d invalid\n", sym_addr[off]);
        return 0;
    }
    kallsyms_num_syms = sym_addr[off+2];
    off += sym_addr[off+1]/4+2;

    printk("3. off=0x%x, type=%d, len=0x%x\n", off*4, sym_addr[off], sym_addr[off+1]);
    if(sym_addr[off] != TYPE_KYMS_NAME){
        printk("TYPE_KYMS_NAME type=%d invalid\n", sym_addr[off]);
        return 0;
    }
    kallsyms_names =(u8_t *) &sym_addr[off+2];
    off += sym_addr[off+1]/4+2;


    printk("4. off=0x%x, type=%d, len=0x%x\n", off*4, sym_addr[off], sym_addr[off+1]);
    if(sym_addr[off] != TYPE_KYMS_MARKERS){
        printk("TYPE_KYMS_MARKERS type=%d invalid\n", sym_addr[off]);
        return 0;
    }
    kallsyms_markers = (long unsigned int *)&sym_addr[off+2];
    off += sym_addr[off+1]/4+2;

    printk("5. off=0x%x, type=%d, len=0x%x\n", off*4, sym_addr[off], sym_addr[off+1]);
    if(sym_addr[off] != TYPE_KYMS_TOKEN_TABEL){
        printk("TYPE_KYMS_TOKEN_TABEL type=%d invalid\n", sym_addr[off]);
        return 0;
    }
    kallsyms_token_table = (u8_t *) &sym_addr[off+2];
    off += sym_addr[off+1]/4+2;

    printk("6. off=0x%x, type=%d, len=0x%x\n", off*4, sym_addr[off], sym_addr[off+1]);
    if(sym_addr[off] != TYPE_KYMS_TOKEN_INDEX){
        printk("TYPE_KYMS_TOKEN_TABEL type=%d invalid\n", sym_addr[off]);
        return 0;
    }
    kallsyms_token_index = (u16_t *) &sym_addr[off+2];
    off += sym_addr[off+1]/4+2;
    return 0;
}

SYS_INIT(print_symb_init, PRE_KERNEL_1, 81);

#endif
