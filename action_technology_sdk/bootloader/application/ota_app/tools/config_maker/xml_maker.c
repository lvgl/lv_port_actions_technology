/*
 * xml_maker.c
 *
 * 配置目标文件转换程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "elf.c"


typedef struct
{
    char      e_name [64];
    char      e_title[128];
    unsigned  e_items;
    char      e_refer[64];
    char      e_attr [128];
    char      comment[128];

} enum_type_t;

typedef struct
{
    char      m_name [64];
    char      m_title[128];
    unsigned  m_value;
    char      m_refer[64];
    char      m_attr [128];
    char      comment[128];

} enum_item_t;

typedef struct
{
    char      s_name [64];
    char      s_title[128];
    unsigned  s_size;
    unsigned  s_items;
    char      s_refer[64];
    char      s_attr [128];
    char      comment[128];

} struct_type_t;

typedef struct
{
    char      m_type [64];
    char      m_name [64];
    char      m_title[128];
    unsigned  m_offs;
    unsigned  m_size;
    unsigned  m_array;
    char      m_refer[64];
    char      m_range[64];
    char      m_attr [128];
    char      comment[128];

} struct_item_t;

typedef struct
{
    unsigned  cfg_id;
    char      c_name [64];
    char      c_title[128];
    unsigned  c_size;
    unsigned  c_items;
    char      c_refer[64];
    char      c_attr [128];
    char      comment[128];

} class_type_t;

typedef struct
{
    char      m_type [64];
    char      m_name [64];
    char      m_title[128];
    unsigned  m_offs;
    unsigned  m_size;
    unsigned  m_array;
    char      m_refer[64];
    char      m_range[64];
    char      m_attr [128];
    char      comment[128];

} class_item_t;


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


typedef struct
{
    char*  obj_file;
    char*  xml_file;
    char*  version;

    int    start_cfg_id;
} xml_maker_t;


static xml_maker_t  xml_maker;


static int get_args(int argc, char* argv[])
{
    xml_maker_t*  p = &xml_maker;

    if (!(argc == 4 || argc == 5))
        goto err;

    p->obj_file = argv[1];
    p->xml_file = argv[2];
    p->version  = argv[3];

    //set start cfg_id if passed
    p->start_cfg_id = 0;
    if (argc == 5)
        p->start_cfg_id = strtol(argv[4], NULL, 16);

    return 0;
err:
    printf("  usage: xml_maker.exe obj_file xml_file version [id]\n");

    return -1;
}


static int get_symbol_type(elf_obj_t* obj, Elf_Sym* sym, char* type)
{
    char*  name;

    if (ELF_ST_TYPE(sym->st_info) != STT_OBJECT ||
        ELF_ST_BIND(sym->st_info) != STB_GLOBAL)
        return -1;

    if (sym->st_shndx == SHN_UNDEF)
        return -1;

    name = &obj->strtab[sym->st_name];

    if (strncmp(name, type, strlen(type)) == 0)
        return 1;

    return -1;
}


static void* get_object_data(elf_obj_t* obj, Elf_Sym* sym)
{
    Elf_Shdr*  shdr;

    void*  cfg_data = malloc(sym->st_size);

    memset(cfg_data, 0, sym->st_size);

    shdr = &obj->shdrtab[sym->st_shndx];

    fseek(obj->file, shdr->sh_offset + sym->st_value, SEEK_SET);

    fread(cfg_data, 1, sym->st_size, obj->file);

    return cfg_data;
}


static int get_config_id(elf_obj_t* obj, Elf_Sym* sym)
{
    int    cfg_id = -1;
    char*  name;

    name = &obj->strtab[sym->st_name];

    if (sscanf(&name[strlen("class_")], "%x", &cfg_id) != 1)
        return -1;

    return cfg_id;
}


static int get_config_num(elf_obj_t* obj)
{
    int  num = 0;
    int  i;

    for (i = 0; i < obj->num_syms; i++)
    {
        Elf_Sym*  sym = &obj->symtab[i];

        if (get_symbol_type(obj, sym, "class_") > 0)
            num += 1;
    }

    return num;
}


static void* get_config_data(elf_obj_t* obj, int cfg_id)
{
    int  i;

    for (i = 0; i < obj->num_syms; i++)
    {
        Elf_Sym*  sym = &obj->symtab[i];

        if (get_symbol_type(obj, sym, "class_") > 0)
        {
            if (get_config_id(obj, sym) == cfg_id)
            {
                return get_object_data(obj, sym);
            }
        }
    }

    printf("  failed %s: 0x%02X\n", __FUNCTION__, cfg_id);

    exit(EXIT_FAILURE);

    return NULL;
}


static int is_mbc_utf8(u8_t* data)
{
    if ((data[0] & 0xE0) == 0xC0 &&
        (data[1] & 0xC0) == 0x80)
    {
        return 2;
    }

    if ((data[0] & 0xF0) == 0xE0 &&
        (data[1] & 0xC0) == 0x80 &&
        (data[2] & 0xC0) == 0x80)
    {
        return 3;
    }

    return 0;
}


static int get_string_width(const char* s)
{
    u8_t*  t = (u8_t*)s;
    int    w = 0;

    while (*t != '\0')
    {
        int  n = is_mbc_utf8(t);

        if (n > 0)
        {
            t += n;
            w += 2;
        }
        else
        {
            t += 1;
            w += 1;
        }
    }

    return w;
}


static int get_width(unsigned data, int type)
{
    char  sbuf[16];
    int   width;

    if (type == 's')
        width = get_string_width((char*)data);

    else if (type == 'x')
        width = sprintf(sbuf, "0x%x", data);

    else if (type == 'u')
        width = sprintf(sbuf, "%u", data);
    else
        width = sprintf(sbuf, "%d", (int)data);

    return width;
}


static int get_align(void* array, int size, int count, int offs, int type)
{
    int  align = 0;
    int  i;

    for (i = 0; i < count; i++)
    {
        void*  data = (char*)array + i * size + offs;
        int    len;

        if (type == 's')
            len = get_string_width(data);
        else
            len = get_width(*(unsigned*)data, type);

        if (align < len)
            align = len;
    }

    return (align + 1);
}


static int get_file_size(elf_obj_t* obj, int num_cfgs, int start_cfg_id)
{
    int  size;
    int  i;

    size = sizeof(config_header_t) + num_cfgs * sizeof(config_info_new_t);

    for (i = 0; i < num_cfgs; i++)
    {
        void*  data = get_config_data(obj, start_cfg_id + i);
        char*  t;

        class_type_t*  c_type = data;

        if ((t = strstr(c_type->c_attr, "fixed_size=")) != NULL)
        {
            c_type->c_size = strtol(t + 11, NULL, 0);
        }

        size += c_type->c_size;

        free(data);
    }

    return size;
}


#define get_align_e(_m, _t)  \
    \
    get_align(e_items, sizeof(enum_item_t), e_type->e_items, offsetof(enum_item_t, _m), _t)

#define get_align_s(_m, _t)  \
    \
    get_align(s_items, sizeof(struct_item_t), s_type->s_items, offsetof(struct_item_t, _m), _t)

#define get_align_c(_m, _t)  \
    \
    get_align(c_items, sizeof(class_item_t), c_type->c_items, offsetof(class_item_t, _m), _t)


#define make_align(_x, _m, _t)  \
do  \
{  \
    int  _a = get_align_##_x(_m, _t);  \
  \
    out_len += sprintf(&out_buf[out_len],  \
        "%*c",  \
        _a - get_width((unsigned)item->_m, _t),  \
        ' ');  \
}  \
while (0)


static int make_category(elf_obj_t* obj, Elf_Sym* sym, char* out_buf)
{
    void*  data = get_object_data(obj, sym);

    enum_type_t*  e_type  = data;
    enum_item_t*  e_items = (void*)((char*)data + sizeof(enum_type_t));

    int  i;
    int  out_len = 0;
    int  hide = 0;

    for (i = 0; i < e_type->e_items; i++)
    {
        enum_item_t*  item = &e_items[i];

        if (strstr(item->m_attr, "hide") != NULL)
            hide += 1;
    }

    out_len += sprintf(&out_buf[out_len],
        "<category num_items=\"%u\">\r\n",
        e_type->e_items - hide);

    for (i = 0; i < e_type->e_items; i++)
    {
        enum_item_t*  item = &e_items[i];

        if (strstr(item->m_attr, "hide") != NULL)
            continue;

        out_len += sprintf(&out_buf[out_len],
            "\t<item name=\"%s\"",
            item->m_name);

        make_align(e, m_name, 's');

        out_len += sprintf(&out_buf[out_len],
            "title=\"%s\"",
            item->m_title);

        make_align(e, m_title, 's');

        out_len += sprintf(&out_buf[out_len],
            "parent=\"%s\"",
            item->m_refer);

        make_align(e, m_refer, 's');

        if (item->m_attr[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "attr=\"%s\" ",
                item->m_attr);
        }

        if (item->comment[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "comment=\"%s\" ",
                item->comment);
        }

        out_len += sprintf(&out_buf[out_len],
            "/>\r\n");
    }

    out_len += sprintf(&out_buf[out_len],
        "</category>\r\n"
        "\r\n");

    free(data);

    return out_len;
}


static int make_enum(elf_obj_t* obj, Elf_Sym* sym, char* out_buf)
{
    void*  data = get_object_data(obj, sym);

    enum_type_t*  e_type  = data;
    enum_item_t*  e_items = (void*)((char*)data + sizeof(enum_type_t));

    int  i;
    int  out_len = 0;
    int  hide = 0;

    if (strcasecmp(e_type->e_name, "category") == 0)
    {
        return make_category(obj, sym, out_buf);
    }

    for (i = 0; i < e_type->e_items; i++)
    {
        enum_item_t*  item = &e_items[i];

        if (strstr(item->m_attr, "hide") != NULL)
            hide += 1;
    }

    out_len += sprintf(&out_buf[out_len],
        "<enum name=\"%s\" title=\"%s\" num_items=\"%u\"",
        e_type->e_name,
        e_type->e_title,
        e_type->e_items - hide);

    if (e_type->e_attr[0] != '\0')
    {
        out_len += sprintf(&out_buf[out_len],
            " attr=\"%s\"",
            e_type->e_attr);
    }

    if (e_type->comment[0] != '\0')
    {
        out_len += sprintf(&out_buf[out_len],
            " comment=\"%s\"",
            e_type->comment);
    }

    out_len += sprintf(&out_buf[out_len],
        ">\r\n");

    for (i = 0; i < e_type->e_items; i++)
    {
        enum_item_t*  item = &e_items[i];

        if (strstr(item->m_attr, "hide") != NULL)
            continue;

        out_len += sprintf(&out_buf[out_len],
            "\t<item name=\"%s\"",
            item->m_name);

        make_align(e, m_name, 's');

        out_len += sprintf(&out_buf[out_len],
            "title=\"%s\"",
            item->m_title);

        make_align(e, m_title, 's');

        out_len += sprintf(&out_buf[out_len],
            "value=\"0x%x\"",
            item->m_value);

        make_align(e, m_value, 'x');

        if (item->m_attr[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "attr=\"%s\" ",
                item->m_attr);
        }

        if (item->comment[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "comment=\"%s\" ",
                item->comment);
        }

        out_len += sprintf(&out_buf[out_len],
            "/>\r\n");
    }

    out_len += sprintf(&out_buf[out_len],
        "</enum>\r\n"
        "\r\n");

    free(data);

    return out_len;
}


static int make_struct(elf_obj_t* obj, Elf_Sym* sym, char* out_buf)
{
    void*  data = get_object_data(obj, sym);

    struct_type_t*  s_type  = data;
    struct_item_t*  s_items = (void*)((char*)data + sizeof(struct_type_t));

    int  i;
    int  out_len = 0;
    int  hide = 0;

    for (i = 0; i < s_type->s_items; i++)
    {
        struct_item_t*  item = &s_items[i];

        if (strstr(item->m_attr, "hide") != NULL)
            hide += 1;
    }

    out_len += sprintf(&out_buf[out_len],
        "<struct name=\"%s\" title=\"%s\" size=\"%u\" num_items=\"%u\"",
        s_type->s_name,
        s_type->s_title,
        s_type->s_size,
        s_type->s_items - hide);

    if (s_type->s_attr[0] != '\0')
    {
        out_len += sprintf(&out_buf[out_len],
            " attr=\"%s\"",
            s_type->s_attr);
    }

    if (s_type->comment[0] != '\0')
    {
        out_len += sprintf(&out_buf[out_len],
            " comment=\"%s\"",
            s_type->comment);
    }

    out_len += sprintf(&out_buf[out_len],
        ">\r\n");

    for (i = 0; i < s_type->s_items; i++)
    {
        struct_item_t*  item = &s_items[i];

        int  align;
        int  a, b;

        if (strstr(item->m_attr, "hide") != NULL)
            continue;

        out_len += sprintf(&out_buf[out_len],
            "\t<item type=\"%s\"",
            item->m_type);

        make_align(s, m_type, 's');

        out_len += sprintf(&out_buf[out_len],
            "name=\"%s\"",
            item->m_name);

        make_align(s, m_name, 's');

        out_len += sprintf(&out_buf[out_len],
            "title=\"%s\"",
            item->m_title);

        make_align(s, m_title, 's');

        out_len += sprintf(&out_buf[out_len],
            "offs=\"%u\"",
            item->m_offs);

        make_align(s, m_offs, 'u');

        out_len += sprintf(&out_buf[out_len],
            "size=\"%u\"",
            item->m_size);

        make_align(s, m_size, 'u');

        a = get_align_s(m_refer, 's');
        b = get_align_s(m_range, 's');

        align = (a > b) ? a : b;

        if (item->m_refer[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "refer=\"%s\"",
                item->m_refer);

            out_len += sprintf(&out_buf[out_len],
                "%*c",
                align - strlen(item->m_refer),
                ' ');
        }
        else
        {
            out_len += sprintf(&out_buf[out_len],
                "range=\"%s\"",
                item->m_range);

            out_len += sprintf(&out_buf[out_len],
                "%*c",
                align - strlen(item->m_range),
                ' ');
        }

        if (strcasecmp(item->m_type, "string") == 0)
        {
            out_len += sprintf(&out_buf[out_len],
                "max_len=\"%u\" ",
                item->m_array);
        }
        else if (item->m_array > 1)
        {
            out_len += sprintf(&out_buf[out_len],
                "array=\"%u\" ",
                item->m_array);
        }

        if (item->m_attr[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "attr=\"%s\" ",
                item->m_attr);
        }

        if (item->comment[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "comment=\"%s\" ",
                item->comment);
        }

        out_len += sprintf(&out_buf[out_len],
            "/>\r\n");
    }

    out_len += sprintf(&out_buf[out_len],
        "</struct>\r\n"
        "\r\n");

    free(data);

    return out_len;
}


static int make_config(void* data, int* offs, char* out_buf)
{
    class_type_t*  c_type  = data;
    class_item_t*  c_items = (void*)((char*)data + sizeof(class_type_t));

    int  i;
    int  out_len = 0;
    int  hide = 0;

    if (strstr(c_type->c_attr, "hide") != NULL)
    {
        goto end;
    }

    for (i = 0; i < c_type->c_items; i++)
    {
        class_item_t*  item = &c_items[i];

        if (strstr(item->m_attr, "hide") != NULL)
            hide += 1;
    }

    out_len += sprintf(&out_buf[out_len],
        "<config name=\"%s\" title=\"%s\" cfg_id=\"0x%02X\" offs=\"0x%x\" size=\"%u\" num_items=\"%u\" category=\"%s\"",
        c_type->c_name,
        c_type->c_title,
        c_type->cfg_id,
        *offs,
        c_type->c_size,
        c_type->c_items - hide,
        c_type->c_refer);

    if (c_type->c_attr[0] != '\0')
    {
        out_len += sprintf(&out_buf[out_len],
            " attr=\"%s\"",
            c_type->c_attr);
    }

    if (c_type->comment[0] != '\0')
    {
        out_len += sprintf(&out_buf[out_len],
            " comment=\"%s\"",
            c_type->comment);
    }

    out_len += sprintf(&out_buf[out_len],
        ">\r\n");

    for (i = 0; i < c_type->c_items; i++)
    {
        class_item_t*  item = &c_items[i];

        int  align;
        int  a, b;

        if (strstr(item->m_attr, "hide") != NULL)
            continue;

        out_len += sprintf(&out_buf[out_len],
            "\t<item type=\"%s\"",
            item->m_type);

        make_align(c, m_type, 's');

        out_len += sprintf(&out_buf[out_len],
            "name=\"%s\"",
            item->m_name);

        make_align(c, m_name, 's');

        out_len += sprintf(&out_buf[out_len],
            "title=\"%s\"",
            item->m_title);

        make_align(c, m_title, 's');

        out_len += sprintf(&out_buf[out_len],
            "offs=\"%u\"",
            item->m_offs);

        make_align(c, m_offs, 'u');

        out_len += sprintf(&out_buf[out_len],
            "size=\"%u\"",
            item->m_size);

        make_align(c, m_size, 'u');

        a = get_align_c(m_refer, 's');
        b = get_align_c(m_range, 's');

        align = (a > b) ? a : b;

        if (item->m_refer[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "refer=\"%s\"",
                item->m_refer);

            out_len += sprintf(&out_buf[out_len],
                "%*c",
                align - strlen(item->m_refer),
                ' ');
        }
        else
        {
            out_len += sprintf(&out_buf[out_len],
                "range=\"%s\"",
                item->m_range);

            out_len += sprintf(&out_buf[out_len],
                "%*c",
                align - strlen(item->m_range),
                ' ');
        }

        if (strcasecmp(item->m_type, "string") == 0)
        {
            out_len += sprintf(&out_buf[out_len],
                "max_len=\"%u\" ",
                item->m_array);
        }
        else if (item->m_array > 1)
        {
            out_len += sprintf(&out_buf[out_len],
                "array=\"%u\" ",
                item->m_array);
        }

        if (item->m_attr[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "attr=\"%s\" ",
                item->m_attr);
        }

        if (item->comment[0] != '\0')
        {
            out_len += sprintf(&out_buf[out_len],
                "comment=\"%s\" ",
                item->comment);
        }

        out_len += sprintf(&out_buf[out_len],
            "/>\r\n");
    }

    out_len += sprintf(&out_buf[out_len],
        "</config>\r\n"
        "\r\n");

end:
    *offs += c_type->c_size;

    free(data);

    return out_len;
}


static int make_xml(void)
{
    xml_maker_t*  p = &xml_maker;

    elf_obj_t*  obj;

    FILE*  xml_file;
    char*  xml_data;
    int    xml_size;

    int  offs;
    int  i;
    int  num_cfgs;
    int  cfg_id = 1;

    if ((obj = elf_obj_load(p->obj_file)) == NULL)
        goto err;

    if ((xml_file = fopen(p->xml_file, "wb")) == NULL)
        goto err;

    //set start cfg_id if it is valid
    if (p->start_cfg_id > 0)
        cfg_id = p->start_cfg_id;

    xml_data = malloc(2*1024*1024);
    xml_size = 0;

    num_cfgs = get_config_num(obj);

    offs = sizeof(config_header_t) + num_cfgs * sizeof(config_info_new_t);

    xml_size += sprintf(&xml_data[xml_size],
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "\r\n"
        "<config_file version=\"%s\" size=\"%u\" num_cfgs=\"%u\">\r\n"
        "\r\n",
        p->version,
        get_file_size(obj, num_cfgs, cfg_id),
        num_cfgs);

    for (i = 0; i < obj->num_syms; i++)
    {
        Elf_Sym*  sym = &obj->symtab[i];

        if (get_symbol_type(obj, sym, "enum_") > 0)
        {
            xml_size += make_enum(obj, sym, &xml_data[xml_size]);
        }

        if (get_symbol_type(obj, sym, "struct_") > 0)
        {
            xml_size += make_struct(obj, sym, &xml_data[xml_size]);
        }
    }

    for (i = 0; i < num_cfgs; i++)
    {
        void*  data = get_config_data(obj, cfg_id + i);
        char*  t;

        class_type_t*  c_type = data;

        if ((t = strstr(c_type->c_attr, "fixed_size=")) != NULL)
        {
            c_type->c_size = strtol(t + 11, NULL, 0);
        }

        xml_size += make_config(data, &offs, &xml_data[xml_size]);
    }

    xml_size += sprintf(&xml_data[xml_size],
        "</config_file>\r\n"
        "\r\n");

    fwrite(xml_data, 1, xml_size, xml_file);

    fclose(xml_file);

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}


int main(int argc, char* argv[])
{
    if (get_args(argc, argv) < 0)
        goto err;

    if (make_xml() < 0)
        goto err;

    return 0;
err:
    return -1;
}


