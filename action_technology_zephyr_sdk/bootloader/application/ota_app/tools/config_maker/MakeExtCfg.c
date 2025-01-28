/*
 * MakeExtCfg.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef unsigned char   u8_t;
typedef unsigned short  u16_t;
typedef unsigned int    u32_t;
typedef unsigned int    uint_t;

#define CFG_ID_MAX_NUM  0x20

#pragma pack(1)

typedef struct
{
    u8_t   format[4];
    u8_t   magic [4];
    u16_t  user_version;
    u8_t   minor_version;
    u8_t   major_version;

    u16_t  total_size;
    u16_t  num_cfgs;

} extcfg_file_header_t;

/*
typedef struct
{
    u8_t  cfg_id;
    u8_t  sub_pos;
    u8_t  cfg_len;
} extcfg_item_info_t;
*/

typedef struct
{
    u32_t  cfg_id:8;
    u32_t  sub_pos:12;
    u32_t  cfg_len:12;
} extcfg_item_info_t;


typedef struct
{
    u32_t id;
    u32_t len;
    u8_t data[0];

} src_data_info_t;

#pragma pack()


typedef struct
{
    char *xml_file_name;
    char *src_file_name;
    char *extcfg_file_name;

    u8_t *xml_data;
    int   xml_data_size;

    u8_t *src_data;
    int   src_data_size;

    u8_t *extcfg_data;
    int   extcfg_data_size;

    extcfg_item_info_t *extcfg_item;
    int  extcfg_item_nums;

    int   dae_cfg_id;
    int   cfg_id_num;
    int   cfg_id[CFG_ID_MAX_NUM];
    int   last_check_id;

    u32_t  cfg_version;
    u32_t  id_item_size;
} make_extcfg_context_t;

static make_extcfg_context_t  make_extcfg_context;


static int get_args(int argc, char* argv[])
{
    make_extcfg_context_t*  ptr = &make_extcfg_context;

    if (argc != 4)
        goto err;

    memset(ptr, 0, sizeof(make_extcfg_context_t));

    ptr->xml_file_name = argv[1];
    ptr->src_file_name = argv[2];
    ptr->extcfg_file_name = argv[3];

    return 0;
err:
    printf("usage: MakeExtCfg.exe xml_file src.bin extcfg.bin\n");
    return -1;
}

static u8_t* read_file(char* file_name, int* file_size)
{
    FILE*  file = NULL;
    u8_t*  data = NULL;
    int    size;

    if ((file = fopen(file_name, "rb")) == NULL)
        goto err;

    fseek(file, 0, SEEK_END);

    size = (int)ftell(file);

    if ((data = malloc(size)) == NULL)
        goto err;

    fseek(file, 0, SEEK_SET);

    if (fread(data, 1, size, file) != size)
        goto err;

    fclose(file);

    if (file_size != NULL)
        *file_size = size;

    return data;

err:
    if (data != NULL)
        free(data);

    if (file != NULL)
        fclose(file);

    return NULL;
}


static int read_file_head(char *file_name, extcfg_file_header_t *head_data, int head_size)
{
    FILE*  file = NULL;
    int file_size = 0;

    file = fopen(file_name, "rb");
    if (file == NULL)
        goto err;

    fseek(file, 0, SEEK_END);
    file_size = (int)ftell(file);
    if (file_size < head_size)
        goto err;

    memset(head_data, 0, head_size);

    fseek(file, 0, SEEK_SET);
    if (fread(head_data, 1, head_size, file) != head_size)
        goto err;

    if ((memcmp(head_data->format, "CFG", 4) != 0) || memcmp(head_data->magic, "VER", 4) != 0)
        goto err;

    fclose(file);
    file = NULL;

    return 0;

err:
    if (file)
        fclose(file);

    return -1;
}


static inline int count_space(char *text)
{
    int j = 0;

    while(text[j] == ' ')
        j++;

    return j;
}

static inline int count_to_end(char *text, char c)
{
    int j = 0;

    while(text[j] != c)
        j++;

    return j;
}

static int get_symbol_value(char *text, int len, char *symbol, char *name)
{
    int i = 0, j = 0;

    if (strncmp(text, symbol, strlen(symbol)) != 0)
        return 0;

    i = strlen(symbol);

    i += count_space(&text[i]);

    if (text[i] != '=')
        return 0;

    i += 1;

    i += count_space(&text[i]);

    if (text[i] != '\"')
        return 0;

    i += 1;

    j = count_to_end(&text[i], '\"');

    memcpy(name, &text[i], j);
    name[j] = '\0';

    return i;
}


static int get_config_info(char *text, int len, char *name, int *id)
{
    int i, j;
    char temp_id[5];

    if (strncmp(text, "<config", 7) != 0)
        return 0;

    i = 7;

    // there is at least 1 space
    if ((j = count_space(&text[i])) == 0)
        return 0;

    i += j;

    // get config name to check if it is Multi Dae ID
    i += get_symbol_value(&text[i], len - i, "name", name);

    while(i < len)
    {
        if ((j = get_symbol_value(&text[i], len - i, "cfg_id", temp_id)) > 0)
        {
            *id = strtol(temp_id, NULL, 16);
            i += j;
            continue;
        }

        if (text[i] == '>')
        {
            i += 1;
            break;
        }

        i++;
    }

    return i;
}


static int get_cfg_id(void)
{
    make_extcfg_context_t*  ptr = &make_extcfg_context;

    int i, j;
    char temp_name[64];
    int temp_id;

    ptr->xml_data = read_file(ptr->xml_file_name, &ptr->xml_data_size);
    if (ptr->xml_data == NULL)
        return -1;

    i = 0;
    ptr->cfg_id_num = 0;
    ptr->dae_cfg_id = 0;
    memset(ptr->cfg_id, 0, sizeof(ptr->cfg_id));

    while(i < ptr->xml_data_size)
    {
        memset(temp_name, 0, sizeof(temp_name));
        temp_id = 0;

        if ((j = get_config_info((char *)&ptr->xml_data[i], ptr->xml_data_size - i, temp_name, &temp_id)) > 0)
        {
            if (strstr(temp_name, "Multi_Dae"))
            {
                ptr->dae_cfg_id = temp_id;
            }
            else
            {
                if (ptr->cfg_id_num >= CFG_ID_MAX_NUM)
                    goto err;

                ptr->cfg_id[ptr->cfg_id_num] = temp_id;
                ptr->cfg_id_num += 1;
            }

            i += j;
            continue;
        }
        i++;
    }

    if (ptr->cfg_id_num == 0 && ptr->dae_cfg_id == 0)
        goto err;

    return 0;
err:
    if (ptr->xml_data != NULL)
        free(ptr->xml_data);

    ptr->xml_data = NULL;
    return -1;
}

static int check_id_valid(u8_t id)
{
    make_extcfg_context_t*  ptr = &make_extcfg_context;

    int i;

    // max custom dae id nums is 10
    if ((id >= ptr->dae_cfg_id) && (id <= ptr->dae_cfg_id + 10 - 1))
        return 1;

    if (ptr->last_check_id == 0)
    {
        // old frimware before transparency mode
        if (ptr->cfg_id_num == 0)
        {
            ptr->last_check_id = id;
            return 1;
        }

        for(i = 0; i < ptr->cfg_id_num; i++)
        {
            if (id == ptr->cfg_id[i])
                return 1;
        }

        // not find the id in config_ext.xml
        ptr->last_check_id = ptr->cfg_id[i-1];
    }

    // is a continuous ID from config_ext.xml
    if (id == (ptr->last_check_id + 1))
    {
        ptr->last_check_id = id;
        return 1;
    }

    return 0;
}

static void extcfg_add(extcfg_item_info_t *item_info, u8_t *data)
{
    make_extcfg_context_t*  ptr = &make_extcfg_context;
    extcfg_item_info_t *item;

    /* write item info */
    ptr->extcfg_item = realloc(ptr->extcfg_item, (ptr->extcfg_item_nums + 1) * ptr->id_item_size);

    item = &ptr->extcfg_item[ptr->extcfg_item_nums];
    item->cfg_id  = item_info->cfg_id;
    item->sub_pos = item_info->sub_pos;
    item->cfg_len = item_info->cfg_len;
    ptr->extcfg_item_nums += 1;


    /* write item data */
    ptr->extcfg_data = realloc(ptr->extcfg_data, ptr->extcfg_data_size + item->cfg_len);
    memcpy(&ptr->extcfg_data[ptr->extcfg_data_size], data, item->cfg_len);
    ptr->extcfg_data_size += item->cfg_len;
}

static int src_data_convert(void)
{
    make_extcfg_context_t *ptr = &make_extcfg_context;
    u8_t *src_data = ptr->src_data;

    src_data_info_t *src_info;
    extcfg_item_info_t item_info;
    int offs = 0;

    while (offs < ptr->src_data_size - 8)
    {
        src_info = (src_data_info_t*)&src_data[offs];

        if (check_id_valid(src_info->id))
        {
            item_info.cfg_id = src_info->id;
            item_info.cfg_len = src_info->len;
            item_info.sub_pos = 0;

            extcfg_add(&item_info, src_info->data);

            /* 8bytes of cfg_id and cfg_len */
            offs += (src_info->len + 8);
            // offset of next data item align with word
            offs = (offs + 3) / 4 * 4;
        }
        else
        {
            /* abort if id wrong */
            return -1;
        }
    }
    return 0;
}


static int convert_extcfg(void)
{
/*  读取版本头信息，从输入的extcfg.bin文件
    逐项转写[ID len data ... ] -> [ID 00 len ... ] [data ...]

    自定义音效的ID是xml中的ID递增，至多10个
    拓展的ID数量只有CFG_ID_MAX_NUM(不包括自定义音效)
*/

    make_extcfg_context_t *ptr = &make_extcfg_context;
    FILE *output_file = NULL;

    extcfg_file_header_t cfg_head;

    ptr->src_data = read_file(ptr->src_file_name, &ptr->src_data_size);
    if (ptr->src_data == NULL)
        goto err;

    // "CFG VER xxxx'size''num'" = 16bytes
    // version number in file_head would change, so we must do copy
    if (read_file_head(ptr->extcfg_file_name, &cfg_head, 0x10) < 0)
        goto err;

    ptr->cfg_version = (u32_t)cfg_head.major_version;
    if (ptr->cfg_version >= 0x20)
    {
        // ptr->id_item_size = sizeof(extcfg_item_info_new_t);
        ptr->id_item_size = sizeof(extcfg_item_info_t);
    }
    else
    {
        ptr->id_item_size = sizeof(extcfg_item_info_t);
    }

    /* convert data */
    if (src_data_convert() < 0)
        goto err;

    cfg_head.num_cfgs = ptr->extcfg_item_nums;
    cfg_head.total_size =
    (   sizeof(extcfg_file_header_t) +
        ptr->id_item_size * ptr->extcfg_item_nums +
        ptr->extcfg_data_size
    );

    /* write data into file */
    output_file = fopen(ptr->extcfg_file_name, "wb");
    if (output_file == NULL)
        goto err;

    fwrite(&cfg_head, 1, sizeof(extcfg_file_header_t), output_file);

    if (ptr->extcfg_item_nums > 0)
    {
        fwrite(ptr->extcfg_item, ptr->id_item_size, ptr->extcfg_item_nums, output_file);
        fwrite(ptr->extcfg_data, 1, ptr->extcfg_data_size, output_file);
    }

    fclose(output_file);
    free(ptr->src_data);
    output_file = NULL;
    ptr->src_data = NULL;

    return 0;

err:
    if (ptr->src_data != NULL)
        free(ptr->src_data);

    if (output_file != NULL)
        fclose(output_file);

    ptr->src_data = NULL;
    output_file = NULL;

    return -1;
}

int main(int argc, char *argv[])
{
    if (get_args(argc, argv) < 0)
        goto err;

    if (get_cfg_id() < 0)
        goto err;

    if (convert_extcfg() < 0)
        goto err;

    return 0;

err:
    printf("fail\n");
    return -1;
}
