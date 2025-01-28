/*
 * xml_source.c
 *
 * 配置源文件转换程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct
{
    char  title  [128];
    char  refer  [64];
    char  range  [64];
    char  attr   [128];
    char  comment[128];

} config_desc_t;


typedef struct
{
    char*  txt_file;
    char*  src_file;

    char*  raw_data;
    int    txt_size;
    char*  txt_data;

    int    cfg_id_start;

} xml_source_t;


static xml_source_t  xml_source;


static inline int get_metachr(char* p, int* m, int* n)
{
    int  i = 1, j = 1;

    if (*p == '\0') return 0;

    if (*p == '\\') i = 2;

    if (*p == '[')
    {
        while (j > 0)
        {
            if (p[i] == '[') j++;
            if (p[i] == ']') j--;

            i += (p[i] == '\\') ? 2 : 1;
        }
    }

    if (m != NULL && n != NULL)
    {
        *m = 1; *n = 1;

        if (p[i] == '*') { *m = 0; *n = -1; i += 1; }
        if (p[i] == '+') { *m = 1; *n = -1; i += 1; }
        if (p[i] == '?') { *m = 0; *n = 1;  i += 1; }
    }

    return i;
}


static int chr_match(char c, char* p)
{
    if (p[0] == '.')
    {
        return (c != '\n') ? 1 : 0;
    }

    if (p[0] == '\\' && p[1] == 'd')
    {
        return chr_match(c, "[0-9]") ? 1 : 0;
    }

    if (p[0] == '\\' && p[1] == 's')
    {
        return chr_match(c, "[ \f\n\r\t\v]") ? 1 : 0;
    }

    if (p[0] == '\\' && p[1] == 'w')
    {
        return chr_match(c, "[A-Za-z0-9_]") ? 1 : 0;
    }

    if (p[0] == '\\')
    {
        return (c == p[1]) ? 1 : 0;
    }

    if (p[0] != '\0' && p[1] == '-')
    {
        return (c >= p[0] && c <= p[2]) ? 1 : 0;
    }

    if (p[0] == '$')
    {
        return (c == '\0') ? 1 : 0;
    }

    if (p[0] == '[' && p[1] != '^')
    {
        int  i = 1;

        while (p[i] != ']')
        {
            if (chr_match(c, &p[i]))
                return 1;

            i += get_metachr(&p[i], NULL, NULL);
        }

        return 0;
    }

    if (p[0] == '[' && p[1] == '^')
    {
        int  i = 2;

        while (p[i] != ']')
        {
            if (chr_match(c, &p[i]))
                return 0;

            i += get_metachr(&p[i], NULL, NULL);
        }

        return 1;
    }

    return (c == p[0]) ? 1 : 0;
}


static inline int str_match_1(char* str, int len, char* p)
{
    int  i, m, n;

    if (get_metachr(p, &m, &n) == 0)
        return 0;

    for (i = 0; i < m && i <= len; i++)
    {
        char  c = (i < len) ? str[i] : '\0';

        if (!chr_match(c, p))
            return -1;
    }

    for (i = m; i != n && i < len; i++)
    {
        if (!chr_match(str[i], p))
            break;
    }

    return (i > len) ? len : i;
}


static int str_match(char* str, int len, char* pattern)
{
    char*  p = pattern;

    int  i, j, k, m, n, r;

    i = j = k = 0;
    m = n = r = 0;

    len = (len <= 0) ? (int)strlen(str) : len;

    while (j < (int)strlen((char*)p))
    {
        r = str_match_1(&str[i], len-i, &p[j]);

        while (r < 0 && i-- > k + m)
        {
            r = str_match_1(&str[i], len-i, &p[j]);
        }

        if (r < 0) return -1;

        j += get_metachr(&p[j], &m, &n);

        k = i; i += r;
    }

    return i;
}


static int get_args(int argc, char* argv[])
{
    xml_source_t*  p = &xml_source;

    if (!(argc == 3 || argc == 4))
        goto err;

    p->txt_file = argv[1];
    p->src_file = argv[2];

    //set cfg_id starting value if passed
    p->cfg_id_start = 0;
    if (argc == 4)
        p->cfg_id_start = strtol(argv[3], NULL, 16);

    return 0;
err:
    printf("  usage: xml_source.exe txt_file src_file [id]\n");

    return -1;
}


static char* load_file(char* file_name, int* file_size)
{
    FILE*  file;
    char*  data;
    int    size;

    if ((file = fopen(file_name, "rb")) == NULL)
        goto err;

    fseek(file, 0, SEEK_END);
    size = ftell(file);

    if ((data = malloc(size + 1)) == NULL)
        goto err;

    fseek(file, 0, SEEK_SET);
    fread(data, 1, size, file);
    fclose(file);

    data[size] = '\0';
    *file_size = size;

    return data;
err:
    printf("  failed %s: %s\n", __FUNCTION__, file_name);

    return NULL;
}


static int strip_comments(char* raw_data, int txt_size, char* out_buf)
{
    char*  s = raw_data;

    int  i = 0;
    int  j = 0;

    while (i < txt_size)
    {
        /* 行注释?
         */
        if (s[i] == '/' && s[i+1] == '/')
        {
            int  n = 2;

            while (s[i+n] != '\0' && s[i+n] != '\r' && s[i+n] != '\n')
                n += 1;

            memset(&out_buf[j], ' ', n);

            i += n;
            j += n;

            continue;
        }

        /* 块注释?
         */
        if (s[i] == '/' && s[i+1] == '*')
        {
            int  n = 2;
            int  k;

            while (s[i+n] != '\0' && !(s[i+n] == '*' && s[i+n+1] == '/'))
                n += 1;

            if (s[i+n] == '*' && s[i+n+1] == '/')
                n += 2;

            for (k = 0; k < n; k++)
            {
                if (s[i+k] == '\t' || s[i+k] == '\r' || s[i+k] == '\n')
                    out_buf[j+k] = s[i+k];
                else
                    out_buf[j+k] = ' ';
            }

            i += n;
            j += n;

            continue;
        }

        /* 字符串?
         */
        if (s[i] == '\"' && ((i == 0) || (i > 0 && s[i-1] != '\\')))
        {
            int  n = 1;

            while (s[i+n] != '\0' && !(s[i+n] == '\"' && s[i+n-1] != '\\'))
                n += 1;

            out_buf[j] = s[i];

            memset(&out_buf[j+1], ' ', n-1);

            i += n;
            j += n;
        }

        out_buf[j++] = s[i++];
    }

    out_buf[j] = '\0';

    return j;
}


static int read_config_file(void)
{
    xml_source_t*  p = &xml_source;

    if ((p->raw_data = load_file(p->txt_file, &p->txt_size)) == NULL)
        goto err;

    if ((p->txt_data = malloc(p->txt_size + 1)) == NULL)
        goto err;

    strip_comments(p->raw_data, p->txt_size, p->txt_data);

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}


static char* get_upper_name(char* name)
{
    static char  upper_name[128];

    int  i;

    for (i = 0; i <= strlen(name); i++)
    {
        upper_name[i] = toupper(name[i]);
    }

    return upper_name;
}


static char* get_short_name(char* name)
{
    if (strncasecmp(name, "CFG_CATEGORY_", 13) == 0)
        return &name[13];

    if (strncasecmp(name, "CFG_", 4) == 0)
        return &name[4];
    else
        return name;
}


static int check_config_type(char* type)
{
    if (strncasecmp(type, "CFG_", 4) == 0)
        return 1;
    else
        return -1;
}


/*
 * simple regular expression replace
 */
extern int str_replace(char* str, int len, char* pattern, char* replace)
{
    int  i = 0, m, n;

    len = (len <= 0) ? (int)strlen(str) : len;

    n = (int)strlen(replace);

    while (i < len)
    {
        if ((m = str_match(&str[i], len-i, pattern)) < 0)
            { i++; continue; }

        if (m != n)
            memmove(&str[i+n], &str[i+m], len-i-m+1);

        if (n > 0)
            memcpy(&str[i], replace, n);

        len += n-m;
        i += n;
    }

    return len;
}


static int get_config_desc(char* text, int len, config_desc_t* desc)
{
    int  i = 0;
    int  j;

    int  attr_len = 0;
    int  end_pos;

    memset(desc, 0, sizeof(config_desc_t));

    if ((j = str_match(&text[i], len - i, "\\s*//\\s*<")) > 0)
        i += j - 1;
    else
        return 0;

    if ((j = str_match(&text[i], len - i, "<[^>]+>")) <= 0)
        return 0;

    i += 1;
    j -= 2;

    end_pos = i+j;

    while (i < end_pos)
    {
        char*  t;

        while (chr_match(text[i], "[\\s,]"))
            i += 1;

        if (text[i] == '/' && text[i+1] == '/')
        {
            i += 2;
            continue;
        }

        if ((j = str_match(&text[i], len - i, "\"[^\"]*\"")) > 0)
        {
            if (j > 2)
            {
                memcpy(desc->title, &text[i+1], j-2);
                desc->title[j-2] = '\0';
            }

            i += j;
            continue;
        }

        if (strncasecmp(&text[i], "CFG_", 4) == 0)
        {
            j = str_match(&text[i], len - i, "\\w+");

            memcpy(desc->refer, &text[i+4], j-4);
            desc->refer[j-4] = '\0';

            if (strncasecmp(desc->refer, "CATEGORY_", 9) == 0)
            {
                memmove(&desc->refer[0], &desc->refer[9], strlen(desc->refer) + 1 - 9);
            }

            i += j;
            continue;
        }

        if ((j = str_match(&text[i], len - i, "[\\-\\.0-9a-fA-Fx]*\\s*~\\s*[\\-\\.0-9a-fA-Fx]*")) > 0)
        {
            int  k;

            if ((k = str_match(&text[i+j], len - (i+j), "\\s*|\\s*[\\-\\.0-9a-fA-Fx]*")) > 0)
                j += k;

            memcpy(desc->range, &text[i], j);
            desc->range[j] = '\0';

            i += j;
            continue;
        }

        if (strncmp(&text[i], "/*", 2) == 0 && (t = strstr(&text[i+2], "*/")) != NULL)
        {
            j = (int)(t - &text[i]) + 2;

            if (j > (len - i))
                break;

            i += 2;
            j -= 2;

            while (chr_match(text[i], "\\s"))
                i++, j--;

            memcpy(desc->comment, &text[i], j-2);

            i += j;
            j -= 2;

            while (j > 0 && chr_match(desc->comment[j-1], "\\s"))
                j--;

            desc->comment[j] = '\0';
            continue;
        }

        while (i < end_pos && text[i] != ',')
        {
            if ((j = str_match(&text[i], len - i, "\"[^\"]*\"")) > 0 ||
                (j = str_match(&text[i], len - i, "\'[^\']*\'")) > 0)
            {
                memcpy(&desc->attr[attr_len], &text[i], j);

                attr_len += j;
                i += j;
                continue;
            }

            desc->attr[attr_len++] = text[i++];
        }

        if (text[i] == ',')
        {
            desc->attr[attr_len++] = text[i++];
            desc->attr[attr_len++] = ' ';
        }
    }

    while (attr_len > 0 && chr_match(desc->attr[attr_len-1], "[\\s,]"))
        attr_len -= 1;

    desc->attr[attr_len] = '\0';

    str_replace(desc->attr, -1, "\"", "\\\"");

    str_replace(desc->title,   -1, "&", "&amp;");
    str_replace(desc->comment, -1, "&", "&amp;");

    return i;
}


static int get_enum(char* text, int len, char* raw_data,

    char* name, config_desc_t* desc, int* m_pos, int* m_len)
{
    int  i, j;

    if (strncmp(&text[0], "enum", 4) != 0)
        return 0;

    i = 4;

    if ((j = str_match(&text[i], len - i, "\\s+")) <= 0)
        return 0;

    i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(name, &text[i], j);
    name[j] = '\0';

    i += j;

    get_config_desc(&raw_data[i], len - i, desc);

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (text[i] != '{')
        return 0;

    i += 1;
    j = 1;

    *m_pos = i;

    /* 定位至相匹配的 '}'
     */
    while (i < len && j > 0)
    {
        if (text[i] == '{')
            j += 1;

        else if (text[i] == '}')
            j -= 1;

        i += 1;
    }

    *m_len = i-1 - *m_pos;

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    i += j;

    if (check_config_type(name) < 0)
    {
        printf("  failed %s: config enum error: \"%s\"\n", __FUNCTION__, name);

        exit(EXIT_FAILURE);
    }

    return i;
}


static int get_enum_item(char* text, int len, char* raw_data, char* name, config_desc_t* desc)
{
    int  i = 0;
    int  j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(name, &text[i], j);
    name[j] = '\0';

    i += j;

    if ((j = str_match(&text[i], len - i, "\\s*,")) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\s*=[^\n]+")) > 0)
        i += j;

    while (i > 0 && chr_match(text[i-1], "\\s"))
        i -= 1;

    get_config_desc(&raw_data[i], len - i, desc);

    return i;
}


static int get_enum_item_num(char* m_pos, int m_len, char* raw_data)
{
    int  num = 0;

    int  i = 0;
    int  n;

    while (i < m_len)
    {
        config_desc_t  desc;

        char  name[128];

        if ((n = get_enum_item(&m_pos[i], m_len - i, &raw_data[i], name, &desc)) > 0)
        {
            num += 1;
            i += n;
            continue;
        }

        i += 1;
    }

    return num;
}


static int make_enum_info(char* e_name, config_desc_t* e_desc,

    char* m_pos, int m_len, char* raw_data, char* out_buf)
{
    int  i = 0;
    int  j = 0;
    int  n;

    int  num_items = get_enum_item_num(m_pos, m_len, raw_data);

    if (e_desc->title[0] == '\0')
        strcpy(e_desc->title, get_short_name(e_name));

    j += sprintf(&out_buf[j],
        "\r\n"
        "\r\n"
        "struct enum_%s_s\r\n"
        "{\r\n"
        "\tenum_type_t  e_type;\r\n"
        "\tenum_item_t  e_items[%u];\r\n"
        "\r\n"
        "} enum_%s =\r\n"
        "{\r\n"
        "\t{ \"%s\", \"%s\", %u, \"%s\", \"%s\", \"%s\", },\r\n"
        "\t{\r\n",
        get_short_name(e_name),
        num_items,
        get_short_name(e_name),
        get_short_name(e_name),
        e_desc->title,
        num_items,
        e_desc->refer,
        e_desc->attr,
        e_desc->comment);

    while (i < m_len)
    {
        config_desc_t  desc;

        char  name[128];

        if ((n = get_enum_item(&m_pos[i], m_len - i, &raw_data[i], name, &desc)) > 0)
        {
            if (desc.title[0] == '\0')
                strcpy(desc.title, get_short_name(name));

            j += sprintf(&out_buf[j],
                "\t\t{ \"%s\",\t\"%s\",\t%s,\t\"%s\",\t\"%s\",\t\"%s\",\t},\r\n",
                get_short_name(name),
                desc.title,
                name,
                desc.refer,
                desc.attr,
                desc.comment);

            i += n;
            continue;
        }

        i += 1;
    }

    j += sprintf(&out_buf[j],
        "\t},\r\n"
        "};");

    return j;
}


static int get_struct(char* text, int len, char* raw_data,

    char* name, config_desc_t* desc, int* m_pos, int* m_len)
{
    int  i, j;

    if (strncmp(&text[0], "struct", 6) != 0)
        return 0;

    i = 6;

    get_config_desc(&raw_data[i], len - i, desc);

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (text[i] != '{')
        return 0;

    i += 1;
    j = 1;

    *m_pos = i;

    /* 定位至相匹配的 '}'
     */
    while (i < len && j > 0)
    {
        if (text[i] == '{')
            j += 1;

        else if (text[i] == '}')
            j -= 1;

        i += 1;
    }

    *m_len = i-1 - *m_pos;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(name, &text[i], j);
    name[j] = '\0';

    i += j;

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    i += j;

    if (check_config_type(name) < 0)
    {
        printf("  failed %s: config struct error: \"%s\"\n", __FUNCTION__, name);

        exit(EXIT_FAILURE);
    }

    return i;
}


static int get_struct_item(char* text, int len, char* raw_data,

    char* type, char* name, char* array, config_desc_t* desc)
{
    int  i = 0;
    int  j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(type, &text[i], j);
    type[j] = '\0';

    i += j;

    if (check_config_type(type) < 0)
    {
        printf("  failed %s: config type error: \"%s\"\n", __FUNCTION__, type);

        exit(EXIT_FAILURE);
    }

    strcpy(array, "1");

    if ((j = str_match(&text[i], len - i, "\\s*([^)\n]+)")) > 0)
    {
        memcpy(array, &text[i], j);
        array[j] = '\0';

        i += j;
    }

    if ((j = str_match(&text[i], len - i, "\\s+")) <= 0)
        return 0;

    i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(name, &text[i], j);
    name[j] = '\0';

    i += j;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\[[^\\]\n]+\\]")) > 0)
    {
        memcpy(array, &text[i], j);
        array[j] = '\0';

        array[0] = '(';
        array[j-1] = ')';

        i += j;
    }

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    i += j;

    get_config_desc(&raw_data[i], len - i, desc);

    return i;
}


static int get_struct_item_num(char* m_pos, int m_len, char* raw_data)
{
    int  num = 0;

    int  i = 0;
    int  n;

    while (i < m_len)
    {
        config_desc_t  desc;

        char  type [128];
        char  name [128];
        char  array[128];

        if ((n = get_struct_item(&m_pos[i], m_len - i, &raw_data[i], type, name, array, &desc)) > 0)
        {
            num += 1;
            i += n;
            continue;
        }

        i += 1;
    }

    return num;
}


static int make_struct_info(char* s_name, config_desc_t* s_desc,

    char* m_pos, int m_len, char* raw_data, char* out_buf)
{
    int  i = 0;
    int  j = 0;
    int  n;

    int  num_items = get_struct_item_num(m_pos, m_len, raw_data);

    if (s_desc->title[0] == '\0')
        strcpy(s_desc->title, get_short_name(s_name));

    j += sprintf(&out_buf[j],
        "\r\n"
        "\r\n"
        "struct struct_%s_s\r\n"
        "{\r\n"
        "\tstruct_type_t  s_type;\r\n"
        "\tstruct_item_t  s_items[%u];\r\n"
        "\r\n"
        "} struct_%s =\r\n"
        "{\r\n"
        "\t{ \"%s\", \"%s\", sizeof(%s), %u, \"%s\", \"%s\", \"%s\", },\r\n"
        "\t{\r\n",
        get_short_name(s_name),
        num_items,
        get_short_name(s_name),
        get_short_name(s_name),
        s_desc->title,
        s_name,
        num_items,
        s_desc->refer,
        s_desc->attr,
        s_desc->comment);

    while (i < m_len)
    {
        config_desc_t  desc;

        char  type [128];
        char  name [128];
        char  array[128];

        if ((n = get_struct_item(&m_pos[i], m_len - i, &raw_data[i], type, name, array, &desc)) > 0)
        {
            if (desc.title[0] == '\0')
                strcpy(desc.title, get_short_name(name));

            j += sprintf(&out_buf[j],
                "\t\t{\t\"%s\", \"%s\", \"%s\",\r\n"
                "\t\t\tCFG_OFFS(%s, %s),\r\n"
                "\t\t\tCFG_SIZE(%s, %s), %s,\r\n"
                "\t\t\t\"%s\", \"%s\", \"%s\", \"%s\",\r\n"
                "\t\t},\r\n",
                get_short_name(type),
                name,
                desc.title,
                s_name,
                name,
                s_name,
                name,
                array,
                desc.refer,
                desc.range,
                desc.attr,
                desc.comment);

            i += n;
            continue;
        }

        i += 1;
    }

    j += sprintf(&out_buf[j],
        "\t},\r\n"
        "};");

    return j;
}


static int get_class(char* text, int len, char* raw_data,

    char* name, config_desc_t* desc, int* n_pos, int* n_len, int* m_pos, int* m_len)
{
    int  i, j;

    if (strncmp(&text[0], "class", 5) != 0)
        return 0;

    i = 5;

    if ((j = str_match(&text[i], len - i, "\\s+")) <= 0)
        return 0;

    i += j;
    *n_pos = i;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(name, &text[i], j);
    name[j] = '\0';

    i += j;
    *n_len = i - *n_pos;

    get_config_desc(&raw_data[i], len - i, desc);

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (text[i] != '{')
        return 0;

    i += 1;
    j = 1;

    *m_pos = i;

    /* 定位至相匹配的 '}'
     */
    while (i < len && j > 0)
    {
        if (text[i] == '{')
            j += 1;

        else if (text[i] == '}')
            j -= 1;

        i += 1;
    }

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    *m_len = i-1 - *m_pos;

    i += j;

    if (check_config_type(name) < 0)
    {
        printf("  failed %s: config class error: \"%s\"\n", __FUNCTION__, name);

        exit(EXIT_FAILURE);
    }

    return i;
}


static int get_class_item(char* text, int len, char* raw_data,

    char* type, char* name, char* array, int* a_pos, config_desc_t* desc)
{
    int  i = 0;
    int  j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(type, &text[i], j);
    type[j] = '\0';

    i += j;

    if (check_config_type(type) < 0)
    {
        printf("  failed %s: config type error: \"%s\"\n", __FUNCTION__, type);

        exit(EXIT_FAILURE);
    }

    strcpy(array, "1");

    if ((j = str_match(&text[i], len - i, "\\s*([^)\n]+)")) > 0)
    {
        memcpy(array, &text[i], j);
        array[j] = '\0';

        i += j;
    }

    if ((j = str_match(&text[i], len - i, "\\s+")) <= 0)
        return 0;

    i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    memcpy(name, &text[i], j);
    name[j] = '\0';

    i += j;
    *a_pos = i;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\[[^\\]\n]+\\]")) > 0)
    {
        memcpy(array, &text[i], j);
        array[j] = '\0';

        array[0] = '(';
        array[j-1] = ')';

        i += j;
        *a_pos = i;
    }

    if ((j = str_match(&text[i], len - i, "\\s*=[^;]+;")) <= 0)
    {
        printf("  failed %s: config not assigned: \"%s\"\n", __FUNCTION__, name);

        exit(EXIT_FAILURE);
    }

    i += j;

    get_config_desc(&raw_data[i], len - i, desc);

    return i;
}


static int get_class_item_num(char* m_pos, int m_len, char* raw_data)
{
    int  num = 0;

    int  i = 0;
    int  n;

    while (i < m_len)
    {
        config_desc_t  desc;

        char  type [128];
        char  name [128];
        char  array[128];
        int   a_pos;

        if ((n = get_class_item(&m_pos[i], m_len - i, &raw_data[i], type, name, array, &a_pos, &desc)) > 0)
        {
            num += 1;
            i += n;
            continue;
        }

        i += 1;
    }

    return num;
}


static int make_class_items(char* m_pos, int m_len, char* raw_data, char* out_buf)
{
    int  i = 0;
    int  j = 0;
    int  n;

    while (i < m_len)
    {
        config_desc_t  desc;

        char  type [128];
        char  name [128];
        char  array[128];
        int   a_pos;

        if ((n = get_class_item(&m_pos[i], m_len - i, &raw_data[i], type, name, array, &a_pos, &desc)) > 0)
        {
            strncpy(&out_buf[j], &raw_data[i], a_pos);
            j += a_pos;

            out_buf[j++] = ';';
            i += n;

            if ((n = str_match(&raw_data[i], m_len - i, "\\s*//")) > 0)
            {
                j += sprintf(&out_buf[j], "\t//");
                i += n;
            }
            continue;
        }

        out_buf[j++] = raw_data[i++];
    }

    return j;
}


static int make_class_info(char* c_name, config_desc_t* c_desc,

    int cfg_id, char* m_pos, int m_len, char* raw_data, char* out_buf)
{
    int  i = 0;
    int  j = 0;
    int  n;

    int  num_items = get_class_item_num(m_pos, m_len, raw_data);

    if (c_desc->title[0] == '\0')
        strcpy(c_desc->title, get_short_name(c_name));

    j += sprintf(&out_buf[j],
        "\r\n"
        "\r\n"
        "struct class_%s_s\r\n"
        "{\r\n"
        "\tclass_type_t  c_type;\r\n"
        "\tclass_item_t  c_items[%u];\r\n"
        "\r\n"
        "} class_%02X_%s =\r\n"
        "{\r\n"
        "\t{ 0x%02X, \"%s\", \"%s\", sizeof(CFG_Struct_%s), %u, \"%s\", \"%s\", \"%s\", },\r\n"
        "\t{\r\n",
        get_short_name(c_name),
        num_items,
        cfg_id,
        get_short_name(c_name),
        cfg_id,
        get_short_name(c_name),
        c_desc->title,
        get_short_name(c_name),
        num_items,
        c_desc->refer,
        c_desc->attr,
        c_desc->comment);

    while (i < m_len)
    {
        config_desc_t  desc;

        char  type [128];
        char  name [128];
        char  array[128];
        int   a_pos;

        if ((n = get_class_item(&m_pos[i], m_len - i, &raw_data[i], type, name, array, &a_pos, &desc)) > 0)
        {
            if (desc.title[0] == '\0')
                strcpy(desc.title, get_short_name(name));

            j += sprintf(&out_buf[j],
                "\t\t{\t\"%s\", \"%s\", \"%s\",\r\n"
                "\t\t\tCFG_OFFS(CFG_Struct_%s, %s),\r\n"
                "\t\t\tCFG_SIZE(CFG_Struct_%s, %s), %s,\r\n"
                "\t\t\t\"%s\", \"%s\", \"%s\", \"%s\",\r\n"
                "\t\t},\r\n",
                get_short_name(type),
                name,
                desc.title,
                get_short_name(c_name),
                name,
                get_short_name(c_name),
                name,
                array,
                desc.refer,
                desc.range,
                desc.attr,
                desc.comment);

            i += n;
            continue;
        }

        i += 1;
    }

    j += sprintf(&out_buf[j],
        "\t},\r\n"
        "};");

    return j;
}


static int make_xml_source(void)
{
    xml_source_t*  p = &xml_source;

    FILE*  src_file;
    char*  src_data;
    int    src_size;

    int    cfg_id = 0;
    int    i, j, k, n;

    char   last_class_name[256] = "";

    // -1: cfg_id would add 1 before it is used
    if (p->cfg_id_start > 0)
        cfg_id = p->cfg_id_start - 1;

    if ((src_data = malloc(2*1024*1024)) == NULL)
        goto err;

    i = 0;
    j = 0;

    j += sprintf(&src_data[j],
        "\r\n"
        "#define CFG_OFFS(_s, _m)  ((unsigned)&((_s*)0)->_m)\r\n"
        "#define CFG_SIZE(_s, _m)  sizeof(((_s*)0)->_m)\r\n"
        "\r\n"
        "typedef struct\r\n"
        "{\r\n"
        "\tchar      e_name [64];\r\n"
        "\tchar      e_title[128];\r\n"
        "\tunsigned  e_items;\r\n"
        "\tchar      e_refer[64];\r\n"
        "\tchar      e_attr [128];\r\n"
        "\tchar      comment[128];\r\n"
        "\r\n"
        "} enum_type_t;\r\n"
        "\r\n"
        "typedef struct\r\n"
        "{\r\n"
        "\tchar      m_name [64];\r\n"
        "\tchar      m_title[128];\r\n"
        "\tunsigned  m_value;\r\n"
        "\tchar      m_refer[64];\r\n"
        "\tchar      m_attr [128];\r\n"
        "\tchar      comment[128];\r\n"
        "\r\n"
        "} enum_item_t;\r\n"
        "\r\n"
        "typedef struct\r\n"
        "{\r\n"
        "\tchar      s_name [64];\r\n"
        "\tchar      s_title[128];\r\n"
        "\tunsigned  s_size;\r\n"
        "\tunsigned  s_items;\r\n"
        "\tchar      s_refer[64];\r\n"
        "\tchar      s_attr [128];\r\n"
        "\tchar      comment[128];\r\n"
        "\r\n"
        "} struct_type_t;\r\n"
        "\r\n"
        "typedef struct\r\n"
        "{\r\n"
        "\tchar      m_type [64];\r\n"
        "\tchar      m_name [64];\r\n"
        "\tchar      m_title[128];\r\n"
        "\tunsigned  m_offs;\r\n"
        "\tunsigned  m_size;\r\n"
        "\tunsigned  m_array;\r\n"
        "\tchar      m_refer[64];\r\n"
        "\tchar      m_range[64];\r\n"
        "\tchar      m_attr [128];\r\n"
        "\tchar      comment[128];\r\n"
        "\r\n"
        "} struct_item_t;\r\n"
        "\r\n"
        "typedef struct\r\n"
        "{\r\n"
        "\tunsigned  cfg_id;\r\n"
        "\tchar      c_name [64];\r\n"
        "\tchar      c_title[128];\r\n"
        "\tunsigned  c_size;\r\n"
        "\tunsigned  c_items;\r\n"
        "\tchar      c_refer[64];\r\n"
        "\tchar      c_attr [128];\r\n"
        "\tchar      comment[128];\r\n"
        "\r\n"
        "} class_type_t;\r\n"
        "\r\n"
        "typedef struct\r\n"
        "{\r\n"
        "\tchar      m_type [64];\r\n"
        "\tchar      m_name [64];\r\n"
        "\tchar      m_title[128];\r\n"
        "\tunsigned  m_offs;\r\n"
        "\tunsigned  m_size;\r\n"
        "\tunsigned  m_array;\r\n"
        "\tchar      m_refer[64];\r\n"
        "\tchar      m_range[64];\r\n"
        "\tchar      m_attr [128];\r\n"
        "\tchar      comment[128];\r\n"
        "\r\n"
        "} class_item_t;\r\n"
        "\r\n");

    while (i < p->txt_size)
    {
        char  name[128];

        config_desc_t  desc;

        int  n_pos, n_len;
        int  m_pos, m_len;

        if ((n = get_enum(&p->txt_data[i], p->txt_size - i, &p->raw_data[i], name, &desc, &m_pos, &m_len)) > 0)
        {
            memcpy(&src_data[j], &p->raw_data[i], n);

            j += n;
            j += make_enum_info(name, &desc, &p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &src_data[j]);

            i += n;
            continue;
        }

        if ((n = get_struct(&p->txt_data[i], p->txt_size - i, &p->raw_data[i], name, &desc, &m_pos, &m_len)) > 0)
        {
            memcpy(&src_data[j], &p->raw_data[i], n);

            j += n;
            j += make_struct_info(name, &desc, &p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &src_data[j]);

            i += n;
            continue;
        }

        if ((n = get_class
            (&p->txt_data[i], p->txt_size - i, &p->raw_data[i], name, &desc, &n_pos, &n_len, &m_pos, &m_len)) > 0)
        {
            if (strcmp(last_class_name, name) != 0)
            {
                strcpy(last_class_name, name);
                cfg_id += 1;
            }

            j += sprintf(&src_data[j],
                "#define CFG_ID_%s  0x%02X\r\n"
                "\r\n",
                get_short_name(get_upper_name(name)),
                cfg_id);

            j += sprintf(&src_data[j], "typedef struct");

            k = m_pos - (n_pos + n_len);

            strncpy(&src_data[j], &p->raw_data[i+n_pos+n_len], k);
            j += k;

            j += make_class_items(&p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &src_data[j]);

            j += sprintf(&src_data[j], "\r\n} CFG_Struct_%s;", get_short_name(name));

            j += make_class_info
                (name, &desc, cfg_id, &p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &src_data[j]);

            i += n;
            continue;
        }

        src_data[j++] = p->raw_data[i++];
    }

    src_size = j;

    if ((src_file = fopen(p->src_file, "wb")) == NULL)
        goto err;

    fwrite(src_data, src_size, 1, src_file);

    fclose(src_file);

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}


int main(int argc, char* argv[])
{
    if (get_args(argc, argv) < 0)
        goto err;

    if (read_config_file() < 0)
        goto err;

    if (make_xml_source() < 0)
        goto err;

    return 0;
err:
    return -1;
}


