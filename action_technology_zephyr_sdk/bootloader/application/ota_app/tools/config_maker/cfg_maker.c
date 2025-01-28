/*
 * cfg_maker.c
 *
 * 配置目标文件转换程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "elf.c"


typedef struct
{
    char*  obj_file;
    char*  bin_file;
    int    major_ver;
    int    minor_ver;
    int    cfg_id_start;

} cfg_maker_t;


typedef struct
{
    u8_t   format[4];
    u8_t   magic [4];
    u16_t  user_version;
    u8_t   minor_version;
    u8_t   major_version;

    u16_t  total_size;
    u16_t  num_cfgs;

} config_header_t;


typedef struct
{
    u8_t  cfg_id;
    u8_t  sub_pos;
    u8_t  cfg_size;

} config_info_t;


typedef struct
{
    u32_t  cfg_id:8;
    u32_t  sub_pos:12;
    u32_t  cfg_size:12;

} config_info_new_t;


static cfg_maker_t  cfg_maker;
u32_t config_item_max_size;

static int get_args(int argc, char* argv[])
{
    cfg_maker_t*  p = &cfg_maker;

    if (!(argc == 5 || argc == 6))
        goto err;

    p->obj_file  = argv[1];
    p->bin_file  = argv[2];
    p->major_ver = atoi(argv[3]);
    p->minor_ver = atoi(argv[4]);

    if (argc == 6) {
        p->cfg_id_start = strtol(argv[5], NULL, 16);
    }

    if (p->major_ver >= 0x20)
    {
        config_item_max_size = 0xFFF;
    }
    else
    {
        config_item_max_size = 0xFF;
    }

    return 0;
err:
    printf("  usage: cfg_maker.exe obj_file bin_file major_ver minor_ver [id]\n");

    return -1;
}


static int get_config_id(elf_obj_t* obj, Elf_Sym* sym, int* fixed_size)
{
    int    cfg_id = -1;
    char*  name;

    if (ELF_ST_TYPE(sym->st_info) != STT_OBJECT ||
        ELF_ST_BIND(sym->st_info) != STB_GLOBAL)
        return -1;

    if (sym->st_shndx == SHN_UNDEF)
        return -1;

    name = &obj->strtab[sym->st_name];

    if (strncmp(name, "ID_", 3) != 0)
        return -1;

    if (sscanf(&name[3], "%x", &cfg_id) != 1)
        return -1;

    if (fixed_size != NULL)
    {
        int  len = (int)strlen(name);

        if (strncmp(&name[len - 6], "_FS_", 4) == 0)
        {
            *fixed_size = strtol(&name[len - 2], NULL, 16);
        }
        else
        {
            *fixed_size = 0;
        }
    }

    return cfg_id;
}


static int get_config_data(elf_obj_t* obj, int cfg_id, void* cfg_data)
{
    Elf_Sym*   sym;
    Elf_Shdr*  shdr;

    int  fixed_size;
    int  i;

    for (i = 0; i < obj->num_syms; i++)
    {
        sym = &obj->symtab[i];

        if (get_config_id(obj, sym, &fixed_size) == cfg_id)
            break;
    }

    if (i >= obj->num_syms)
        goto err;

    if (sym->st_size > config_item_max_size)
    {
        printf("  failed: sizeof(%s) > %d\n", &obj->strtab[sym->st_name], config_item_max_size);
        goto err;
    }

    if (fixed_size > 0 &&
        sym->st_size > fixed_size)
    {
        printf("  failed: sizeof(%s) > %d\n", &obj->strtab[sym->st_name], fixed_size);
        goto err;
    }

    memset(cfg_data, 0, sym->st_size);

    shdr = &obj->shdrtab[sym->st_shndx];

    if (shdr->sh_type == SHT_NOBITS)
    {
        return (fixed_size > 0) ? fixed_size : sym->st_size;
    }

    if (shdr->sh_type != SHT_PROGBITS)
        goto err;

    fseek(obj->file, shdr->sh_offset + sym->st_value, SEEK_SET);

    fread(cfg_data, 1, sym->st_size, obj->file);

    return (fixed_size > 0) ? fixed_size : sym->st_size;

err:
    printf("  failed %s: ID_%02X\n", __FUNCTION__, cfg_id);

    return -1;
}


static int get_config_num(elf_obj_t* obj)
{
    int  num = 0;
    int  i;

    for (i = 0; i < obj->num_syms; i++)
    {
        Elf_Sym*  sym = &obj->symtab[i];

        if (get_config_id(obj, sym, NULL) > 0)
            num += 1;
    }

    return num;
}


static int make_config_bin(void)
{
    cfg_maker_t*  p = &cfg_maker;

    elf_obj_t*  obj;

    static char  bin_data[0x10000];

    config_header_t*  cfg_hdr;
    config_info_t*    cfg_info;
    config_info_new_t* new_cfg_info;

    FILE*  file;
    int    i;

    if ((obj = elf_obj_load(p->obj_file)) == NULL)
        goto err;

    cfg_hdr = (config_header_t*)&bin_data[0];

    cfg_info = (config_info_t*)&bin_data[sizeof(config_header_t)];
    new_cfg_info = (config_info_new_t*)&bin_data[sizeof(config_header_t)];

    memcpy(cfg_hdr->format, "CFG", 4);
    memcpy(cfg_hdr->magic,  "VER", 4);

    cfg_hdr->user_version  = p->minor_ver | (p->major_ver << 8);
    cfg_hdr->minor_version = p->minor_ver;
    cfg_hdr->major_version = p->major_ver;

    cfg_hdr->num_cfgs = get_config_num(obj);

    if (p->major_ver >= 0x20)
    {
        cfg_hdr->total_size = sizeof(config_header_t) + cfg_hdr->num_cfgs * sizeof(config_info_new_t);
    }
    else
    {
        cfg_hdr->total_size = sizeof(config_header_t) + cfg_hdr->num_cfgs * sizeof(config_info_t);
    }

    for (i = 0; i < cfg_hdr->num_cfgs; i++)
    {
        void*  cfg_data = &bin_data[cfg_hdr->total_size];
        int    cfg_size;
        int    cfg_id = (i + 1);

        if (p->cfg_id_start > 0)
            cfg_id = p->cfg_id_start + i;

        if ((cfg_size = get_config_data(obj, cfg_id, cfg_data)) < 0)
            goto err;

        if (p->major_ver >= 0x20)
        {
            new_cfg_info[i].cfg_id   = cfg_id;
            new_cfg_info[i].sub_pos  = 0;
            new_cfg_info[i].cfg_size = cfg_size;
        }
        else
        {
            cfg_info[i].cfg_id   = cfg_id;
            cfg_info[i].sub_pos  = 0;
            cfg_info[i].cfg_size = cfg_size;
        }

        cfg_hdr->total_size += cfg_size;
    }

    if ((file = fopen(p->bin_file, "wb")) == NULL)
        goto err;

    fwrite(bin_data, 1, cfg_hdr->total_size, file);

    fclose(file);

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}


int main(int argc, char* argv[])
{
    if (get_args(argc, argv) < 0)
        goto err;

    if (make_config_bin() < 0)
        goto err;

    return 0;
err:
    return -1;
}


