/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author      Notes
 * 2018/08/29     Bernard     first version
 */

#include <zephyr.h>
#include <init.h>
#include <sys/sys_heap.h>
#include <fs/fs.h>
#include <shell/shell.h>

#include "dlfcn.h"
#include "dlmodule.h"
#include "dlelf.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(DLMD, LOG_LEVEL_INF);

static struct rt_module_symtab *_rt_module_symtab_begin = RT_NULL;
static struct rt_module_symtab *_rt_module_symtab_end   = RT_NULL;

static rt_list_t _rt_module_list;

#if defined(__IAR_SYSTEMS_ICC__) /* for IAR compiler */
    #pragma section="RTMSymTab"
#endif

/* get the base name of path */
static void _rt_module_get_basename(char *name, const char *path)
{
    int size;
    const char *first, *end, *ptr;

    ptr   = first = (char *)path;
    end   = path + rt_strlen(path);

    while (*ptr != '\0')
    {
        if (*ptr == '/')
            first = ptr + 1;
        if (*ptr == '.')
            end = ptr - 1;

        ptr ++;
    }

    size = end - first + 1;
    if (size > RT_NAME_MAX) size = RT_NAME_MAX;

    rt_strncpy(name, first, size);
    name[size] = '\0';
}

#define RT_MODULE_ARG_MAX    8
static int _rt_module_split_arg(char *cmd, rt_size_t length, char *argv[])
{
    int argc = 0;
    char *ptr = cmd;

    while ((ptr - cmd) < length)
    {
        /* strip bank and tab */
        while ((*ptr == ' ' || *ptr == '\t') && (ptr - cmd) < length)
            *ptr++ = '\0';
        /* check whether it's the end of line */
        if ((ptr - cmd) >= length) break;

        /* handle string with quote */
        if (*ptr == '"')
        {
            argv[argc++] = ++ptr;

            /* skip this string */
            while (*ptr != '"' && (ptr - cmd) < length)
                if (*ptr ++ == '\\')  ptr ++;
            if ((ptr - cmd) >= length) break;

            /* skip '"' */
            *ptr ++ = '\0';
        }
        else
        {
            argv[argc++] = ptr;
            while ((*ptr != ' ' && *ptr != '\t') && (ptr - cmd) < length)
                ptr ++;
        }

        if (argc >= RT_MODULE_ARG_MAX) break;
    }

    return argc;
}

static void _dlmodule_thread_entry(void *p1, void *p2, void *p3)
{
    int argc = 0;
    char *argv[RT_MODULE_ARG_MAX];

    struct rt_dlmodule *module = (struct rt_dlmodule*)p1;

    if (module == RT_NULL || module->cmd_line == RT_NULL)
        /* malloc for module_cmd_line failed. */
        return;

    if (module->cmd_line)
    {
        rt_memset(argv, 0x00, sizeof(argv));
        argc = _rt_module_split_arg((char *)module->cmd_line, rt_strlen(module->cmd_line), argv);
        if (argc == 0) goto __exit;
    }

    /* set status of module */
    module->stat = RT_DLMODULE_STAT_RUNNING;

    LOG_D("run main entry: 0x%p with %s",
        module->entry_addr,
        module->cmd_line);

    if (module->entry_addr)
        module->entry_addr(argc, argv);

    /* set status of module */
    module->stat = RT_DLMODULE_STAT_CLOSING;

__exit:
    rt_free(module->main_stack);
    if (module->exec_load)
    {
        dlmodule_destroy(module);
    }
    return ;
}

/**
 * @brief create a dynamic module object and initialize it.
 *
 * @return struct rt_dlmodule* If module create successfully, return the pointer to its rt_dlmodule structure.
 */
struct rt_dlmodule *dlmodule_create(void)
{
    struct rt_dlmodule *module = RT_NULL;

    module = (struct rt_dlmodule*) rt_malloc(sizeof(struct rt_dlmodule));
    if (module)
    {
        memset(module, 0, sizeof(struct rt_dlmodule));
        module->stat = RT_DLMODULE_STAT_INIT;

        /* set initial priority and stack size */
        module->priority = RT_THREAD_PRIORITY_MAX - 1;
        module->stack_size = 2048;

        rt_list_init(&module->node);
        rt_list_insert_after(&_rt_module_list, &module->node);
    }

    return module;
}

/**
 * @brief destroy dynamic module and cleanup all kernel objects inside it.
 *
 * @param module Pointer to the module to be destroyed.
 * @return rt_err_t  On success, it returns RT_EOK. Otherwise, it returns the error code.
 */
rt_err_t dlmodule_destroy(struct rt_dlmodule* module)
{
    int i;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* check parameter */
    if (module == RT_NULL)
        return -RT_ERROR;

    /* can not destroy a running module */
    if (module->stat == RT_DLMODULE_STAT_RUNNING)
        return -RT_EBUSY;

    /* do module cleanup */
    if (module->cleanup_func)
    {
        rt_enter_critical();
        module->cleanup_func(module);
        rt_exit_critical();
    }

    if (module->cmd_line) rt_free(module->cmd_line);
    /* release module symbol table */
    for (i = 0; i < module->nsym; i ++)
    {
        rt_free((void *)module->symtab[i].name);
    }
    if (module->symtab != RT_NULL)
    {
        rt_free(module->symtab);
    }

    /* destory module */
    rt_free(module->mem_space);
    /* delete module object */
    rt_list_remove(&module->node);
    rt_free(module);

    return RT_EOK;
}

/**
 * @brief load an ELF module to memory.
 *
 * @param filename the path to the module to load.
 * @return struct rt_dlmodule* On success, it returns a pointer to the module object. otherwise, RT_NULL is returned.
 *
 * @note the function is used to load an ELF (Executable and Linkable Format) module from a file, validate it,
 *       and initialize it as a dynamically loaded module. what it implements are as follows:
 *       1. Load and Validate ELF: It loads an ELF file, checks its validity, and identifies it as either a relocatable or shared object.
 *       2. Memory Allocation and Cleanup: Uses rt_malloc and rt_free to allocate and free memory for module data.
 *          Error handling ensures all resources are released if an error occurs.
 *       3. Symbol Resolution and Initialization: Sets up init function and cleanup function, and calls the module_init function if it is present.
 *       4. Cache Management: Optionally (when RT_USING_CACHE defined) flushes data and invalidates instruction caches to ensure the module is correctly loaded into memory.
 */
struct rt_dlmodule* dlmodule_load(const char* filename)
{
    struct fs_file_t file;
    int length = 0;
    rt_err_t ret = RT_EOK;
    rt_uint8_t *module_ptr = RT_NULL;
    struct rt_dlmodule *module = RT_NULL;

    memset(&file, 0, sizeof(struct fs_file_t));
    ret = fs_open(&file, filename, FS_O_READ);
    if (ret == RT_EOK)
    {
        fs_seek(&file, 0, SEEK_END);
        length = fs_tell(&file);
        fs_seek(&file, 0, SEEK_SET);

        if (length == 0) goto __exit;

        module_ptr = (uint8_t*) rt_malloc (length);
        if (!module_ptr) goto __exit;

        if (fs_read(&file, module_ptr, length) != length)
            goto __exit;

        /* close file and release fd */
        fs_close(&file);
    }
    else
    {
        rt_kprintf("Module: open %s error\n", filename);
        goto __exit;
    }

    if (!module_ptr) goto __exit;

    /* check ELF header */
    if (rt_memcmp(elf_module->e_ident, RTMMAG, SELFMAG) != 0 &&
        rt_memcmp(elf_module->e_ident, ELFMAG, SELFMAG) != 0)
    {
        rt_kprintf("Module: magic error\n");
        goto __exit;
    }

    /* check ELF class */
    if ((elf_module->e_ident[EI_CLASS] != ELFCLASS32)&&(elf_module->e_ident[EI_CLASS] != ELFCLASS64))
    {
        rt_kprintf("Module: ELF class error\n");
        goto __exit;
    }

    module = dlmodule_create();
    if (!module) goto __exit;

    /* set the name of module */
    _rt_module_get_basename(module->name, filename);

    LOG_D("rt_module_load: %.*s", RT_NAME_MAX, filename);

    if (elf_module->e_type == ET_REL)
    {
        ret = dlmodule_load_relocated_object(module, module_ptr);
    }
    else if (elf_module->e_type == ET_DYN)
    {
        ret = dlmodule_load_shared_object(module, module_ptr);
    }
    else
    {
        rt_kprintf("Module: unsupported elf type\n");
        goto __exit;
    }

    /* check return value */
    if (ret != RT_EOK) goto __exit;

    /* release module data */
    rt_free(module_ptr);

    /* increase module reference count */
    module->nref ++;

    /* set module initialization and cleanup function */
    module->init_func = dlsym(module, "module_init");
    module->cleanup_func = dlsym(module, "module_cleanup");
    module->stat = RT_DLMODULE_STAT_INIT;
    /* do module initialization */
    if (module->init_func)
    {
        module->init_func(module);
    }

    return module;

__exit:
    if (module_ptr) rt_free(module_ptr);
    if (module) dlmodule_destroy(module);

    return RT_NULL;
}

/**
 * @brief load a dynamic module, and create a thread to excute the module main function.
 *
 * @param pgname path of the module to be loaded.
 * @param cmd the command string (with commandline options) for startup module.
 * @param cmd_size the command's length.
 * @return struct rt_dlmodule* On success, it returns a pointer to the module object. otherwise, RT_NULL is returned.
 */
struct rt_dlmodule* dlmodule_exec(const char* pgname, const char* cmd, int cmd_size)
{
    struct rt_dlmodule *module = RT_NULL;

    module = dlmodule_find(pgname);
    if (module == RT_NULL)
    {
        module = dlmodule_load(pgname);
        if (module)
        {
            module->exec_load = 1; /* destroy module after main exit */
        }
    }
    if (module)
    {
        if (module->entry_addr)
        {
            /* exec this module */
            k_tid_t tid;

            module->cmd_line = rt_malloc(strlen(pgname) + 1 + cmd_size);
            if (!module->cmd_line) goto __exit;
            rt_strcpy(module->cmd_line, pgname);
            if (cmd)
            {
                rt_strcat(module->cmd_line, " ");
                rt_strcat(module->cmd_line, cmd);
            }

            /* check stack size and priority */
            if (module->priority > RT_THREAD_PRIORITY_MAX) module->priority = RT_THREAD_PRIORITY_MAX - 1;
            if (module->stack_size < 2048 || module->stack_size > (1024 * 32)) module->stack_size = 2048;

            module->main_stack = rt_malloc(module->stack_size);
            if (module->main_stack == RT_NULL) goto __exit;

            tid = k_thread_create(&module->main_thread, (k_thread_stack_t*)module->main_stack, module->stack_size,
                _dlmodule_thread_entry, (void*)module, NULL, NULL, module->priority, 0, K_NO_WAIT);
            if (tid)
            {
                k_thread_name_set(tid, module->name);
                module->main_tid = tid;
            }
            else
            {
                goto __exit;
            }
        }
    }

    return module;

__exit:
    if (module->main_stack) rt_free(module->main_stack);
    if (module) dlmodule_destroy(module);

    return RT_NULL;
}

/**
 * @brief search for a symbol by its name in the kernel symbol table.
 *
 * @param sym_str the symbol name string.
 * @return rt_uint32_t On success, it returns the address of the symbol.
 *         Otherwise, it returns 0 (indicating the symbol was not found).
 */
rt_uint32_t dlmodule_symbol_find(const char *sym_str)
{
    /* find in kernel symbol table */
    struct rt_module_symtab *index;

    for (index = _rt_module_symtab_begin; index != _rt_module_symtab_end; index ++)
    {
        if (rt_strcmp(index->name, sym_str) == 0)
            return (rt_uint32_t)index->addr;
    }

    return 0;
}

int rt_system_dlmodule_init(const struct device *arg)
{
#if defined(__GNUC__) && !defined(__CC_ARM)
    extern int __rtmsymtab_start;
    extern int __rtmsymtab_end;

    _rt_module_symtab_begin = (struct rt_module_symtab *)&__rtmsymtab_start;
    _rt_module_symtab_end   = (struct rt_module_symtab *)&__rtmsymtab_end;
#elif defined (__CC_ARM)
    extern int RTMSymTab$$Base;
    extern int RTMSymTab$$Limit;

    _rt_module_symtab_begin = (struct rt_module_symtab *)&RTMSymTab$$Base;
    _rt_module_symtab_end   = (struct rt_module_symtab *)&RTMSymTab$$Limit;
#elif defined (__IAR_SYSTEMS_ICC__)
    _rt_module_symtab_begin = __section_begin("RTMSymTab");
    _rt_module_symtab_end   = __section_end("RTMSymTab");
#endif

    rt_list_init(&_rt_module_list);
    dlheap_init();

    return 0;
}

SYS_INIT(rt_system_dlmodule_init, POST_KERNEL, 99);

/**
 * This function will find the specified module.
 *
 * @param name the name of module finding
 *
 * @return the module
 */
struct rt_dlmodule *dlmodule_find(const char *name)
{
    char basename[RT_NAME_MAX];
    struct rt_dlmodule *module;
    struct rt_list_node *node;

    _rt_module_get_basename(basename, name);

    for (node = _rt_module_list.next; node != &_rt_module_list; node = node->next)
    {
        module = (struct rt_dlmodule *)(rt_list_entry(node, struct rt_dlmodule, node));
        if (!rt_strcmp(module->name, basename))
        {
            return module;
        }
    }

    return RT_NULL;
}

RTM_EXPORT(dlmodule_find);

static int shell_cmd_list_symbols(const struct shell *shell, size_t argc, char **argv)
{
    extern int __rtmsymtab_start;
    extern int __rtmsymtab_end;

    /* find in kernel symbol table */
    struct rt_module_symtab *index;

    for (index = _rt_module_symtab_begin;
         index != _rt_module_symtab_end;
         index ++)
    {
        shell_print(shell, "%s => 0x%08x\n", index->name, index->addr);
    }

    return 0;
}

static int shell_cmd_list_module(const struct shell *shell, size_t argc, char **argv)
{
    struct rt_dlmodule *module;
    struct rt_list_node *node;

    for (node = _rt_module_list.next; node != &_rt_module_list; node = node->next)
    {
        module = (struct rt_dlmodule *)(rt_list_entry(node, struct rt_dlmodule, node));
        shell_print(shell, "%-*.*s %-04d  0x%08x\n",
                   RT_NAME_MAX, RT_NAME_MAX, module->name, module->nref, module->mem_space);
    }

    dlheap_dump();

    return 0;
}

static int shell_cmd_load_module(const struct shell *shell, size_t argc, char **argv)
{
    struct rt_dlmodule *module;

    if (argc >= 2)
    {
        module = dlmodule_find(argv[1]);
        if (module == RT_NULL)
        {
            module = dlmodule_load(argv[1]);
            shell_print(shell, "load module: %s -> 0x%x\n", argv[1], (int)module);
        }
        else
        {
            shell_print(shell, "module: %s already present\n", argv[1]);
        }
    }

    return 0;
}

static int shell_cmd_unload_module(const struct shell *shell, size_t argc, char **argv)
{
    struct rt_dlmodule *module;

    if (argc >= 2)
    {
        module = dlmodule_find(argv[1]);
        if (module != RT_NULL)
        {
            dlmodule_destroy(module);
            shell_print(shell, "unload module: %s\n", argv[1]);
        }
        else
        {
            shell_print(shell, "module: %s not found\n", argv[1]);
        }
    }

    return 0;
}

static int shell_cmd_exec_module(const struct shell *shell, size_t argc, char **argv)
{
    char* cmd = NULL;
    int cmd_size = 0;

    if (argc >= 2)
    {
        if (argc >=3)
        {
            cmd = argv[2];
            cmd_size = strlen(cmd) + 1;
        }
        dlmodule_exec(argv[1], cmd, cmd_size);
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rtm_sub,
    SHELL_CMD(list_symbols, NULL, "list symbols", shell_cmd_list_symbols),
    SHELL_CMD(list_module, NULL, "list modules", shell_cmd_list_module),
    SHELL_CMD(load_module, NULL, "load module", shell_cmd_load_module),
    SHELL_CMD(unload_module, NULL, "unload module", shell_cmd_unload_module),
    SHELL_CMD(exec_module, NULL, "exec module", shell_cmd_exec_module),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(rtm, &rtm_sub, "rtm commands", NULL);

