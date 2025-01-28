/*
 * cfg_source.c
 *
 * 配置源文件转换程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct
{
    char*  txt_file;
    char*  hdr_file;
    char*  src_file;

    char*  raw_data;
    int    txt_size;
    char*  txt_data;

    int    err_count;
    int    cfg_id_start;

} cfg_source_t;


static cfg_source_t  cfg_source;


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
    cfg_source_t*  p = &cfg_source;

    if (!(argc == 4 || argc == 5))
        goto err;

    p->txt_file = argv[1];
    p->hdr_file = argv[2];
    p->src_file = argv[3];

    p->cfg_id_start = 0;
    if (argc == 5) {
        p->cfg_id_start = strtol(argv[4], NULL, 16);
    }

    return 0;
err:
    printf("  usage: cfg_source.exe txt_file hdr_file src_file [id]\n");

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


/*!
 * \brief 字符串替换
 */
extern int str_replace(char* str_buf, int buf_size, int src_pos, int src_len, char* dst_str)
{
    int  src_size = strlen(&str_buf[src_pos]) + 1;

    int  dst_size;
    int  dst_len;

    if (buf_size < src_pos + src_size)
        buf_size = src_pos + src_size;

    dst_size = buf_size - src_pos - (src_size - src_len);

    dst_len = (dst_str != NULL) ? strlen(dst_str) : 0;

    if (dst_len > dst_size)
        dst_len = dst_size;

    memmove(&str_buf[src_pos + dst_len], &str_buf[src_pos + src_len], src_size - src_len);

    memcpy(&str_buf[src_pos], dst_str, dst_len);

    return dst_len;
}


/* 环境变量赋值替换
 */
int env_replace(char* str_buf, int buf_size)
{
    int  i = 0;
    int  n;

    while (i < buf_size && str_buf[i] != '\0')
    {
        char   env_name[128];
        char*  env_value;
        char*  t;

        if (!(str_buf[i] == '$' && str_buf[i+1] == '('))
        {
            i += 1;
            continue;
        }

        if ((t = strchr(&str_buf[i+2], ')')) == NULL)
        {
            i += 1;
            continue;
        }

        n = (int)(t - &str_buf[i+2]);

        strncpy(env_name, &str_buf[i+2], n);

        env_name[n] = '\0';
        env_value = getenv(env_name);

        i += str_replace(str_buf, buf_size, i, n + 3, env_value);
    }

    return i;
}


static int read_config_file(void)
{
    cfg_source_t*  p = &cfg_source;

    if ((p->raw_data = load_file(p->txt_file, &p->txt_size)) == NULL)
        goto err;

    p->txt_size = env_replace(p->raw_data, p->txt_size);

    if ((p->txt_data = malloc(p->txt_size + 1)) == NULL)
        goto err;

    strip_comments(p->raw_data, p->txt_size, p->txt_data);

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}


static char* get_short_name(char* name)
{
    if (strncasecmp(name, "CFG_", 4) == 0)
        return &name[4];
    else
        return name;
}


static int get_class(char* text, int len, int* n_pos, int* n_len, int* m_pos, int* m_len)
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

    i += j;
    *n_len = i - *n_pos;

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

    return i;
}


static int get_member(char* text, int len, int* n_pos, int* n_len,

    int* a_pos, int* a_len, int* b_pos, int* b_len)
{
    int  i = 0;
    int  j;

    if ((j = str_match(&text[i], len - i, "\\w+[\\s\\*]+")) <= 0)
        return 0;

    i += j;
    *n_pos = i;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    i += j;
    *n_len = i - *n_pos;

    *a_pos = i;
    *a_len = 0;

    *b_pos = i;
    *b_len = 0;

    if ((j = str_match(&text[i], len - i, "\\s*\\[[^\\]]+\\]")) > 0)
    {
        i += j;
        *a_pos = i;
    }

    if ((j = str_match(&text[i], len - i, "\\s*:\\s*\\d+")) > 0)
    {
        i += j;
        *b_len = i - *b_pos;
        *a_pos = i;
    }

    if ((j = str_match(&text[i], len - i, "\\s*=[^;]+;")) > 0)
    {
        i += j;
        *a_len = i - *a_pos;

        return i;
    }

    if ((j = str_match(&text[i], len - i, "\\s*;")) > 0)
    {
        i += j;
        *a_len = 0;

        return i;
    }

    return 0;
}


static int get_align(char* text, int len)
{
    int  align = 0;

    int  i = 0;
    int  n;

    while (i < len)
    {
        int  n_pos, n_len;
        int  a_pos, a_len;
        int  b_pos, b_len;

        if ((n = get_member(&text[i], len - i, &n_pos, &n_len, &a_pos, &a_len, &b_pos, &b_len)) > 0)
        {
            if (align < a_pos)
                align = a_pos;

            i += n;
            continue;
        }

        i += 1;
    }

    return (align + 1);
}


static int make_members(char* text, int len, char* raw_data, char* out_buf)
{
    int  i = 0;
    int  j = 0;
    int  n;

    int  align = get_align(text, len);

    while (i < len)
    {
        int  n_pos, n_len;
        int  a_pos, a_len;
        int  b_pos, b_len;

        if ((n = get_member(&text[i], len - i, &n_pos, &n_len, &a_pos, &a_len, &b_pos, &b_len)) > 0)
        {
            strncpy(&out_buf[j], &raw_data[i], a_pos);
            j += a_pos;

            out_buf[j++] = ';';
            i += n;

            if ((n = str_match(&raw_data[i], len - i, "\\s*//")) > 0)
            {
                j += sprintf(&out_buf[j], "%*c //", (align - a_pos), ' ');
                i += n;
            }
            continue;
        }

        out_buf[j++] = raw_data[i++];
    }

    return j;
}


static int make_assigns(char* text, int len, char* raw_data, char* out_buf)
{
    cfg_source_t*  p = &cfg_source;

    int  i = 0;
    int  j = 0;
    int  n;

    while (i < len)
    {
        int  n_pos, n_len;
        int  a_pos, a_len;
        int  b_pos, b_len;

        if ((n = get_member(&text[i], len - i, &n_pos, &n_len, &a_pos, &a_len, &b_pos, &b_len)) > 0)
        {
            if (a_len <= 0)
            {
                char  member_name[256];

                j += sprintf(&out_buf[j], "// ");

                strncpy(&out_buf[j], &raw_data[i], n);
                j += n;

                strncpy(member_name, &raw_data[i+n_pos], n_len);
                member_name[n_len] = '\0';

                printf("  error: not assigned: '%s'\n", member_name);

                p->err_count += 1;
            }
            else
            {
                int  k;

                out_buf[j++] = '.';

                strncpy(&out_buf[j], &raw_data[i+n_pos], n_len);
                j += n_len;

                out_buf[j++] = '\t';

                if ((k = str_match(&raw_data[i+a_pos], a_len, "\\s*")) > 0)
                {
                    a_pos += k;
                    a_len -= k;
                }

                strncpy(&out_buf[j], &raw_data[i+a_pos], a_len - 1);
                j += a_len - 1;

                out_buf[j++] = ',';
            }

            i += n;

            if ((n = str_match(&raw_data[i], len - i, "\\s*//")) > 0)
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


static int strip_cfg_comment(char* line_comment)
{
    char*  s = line_comment;
    char   t[1024];

    int    i = 0;
    int    j = 0;

    while (s[i] == '/' || s[i] == ' ' || s[i] == '\t')
    {
        t[j++] = s[i++];
    }

    if (s[i] == '<')
    {
        i += 1;
    }

    while (s[i] != '\0')
    {
        int  m = 0;

        while (s[i] == ',' || s[i] == ' ' || s[i] == '\t')
        {
            i += 1;
            m += 1;
        }

        if (s[i] == '\"')
        {
            memcpy(&t[j], &s[i-m], m);
            j += m;

            i += 1;

            while (s[i] != '\"')
            {
                t[j++] = s[i++];
            }

            i += 1;
            continue;
        }

        if ((strncmp(&s[i], "CFG", 3) == 0 ||
             strncmp(&s[i], "cfg", 3) == 0)
            &&
            strncmp(&s[i], "CFG_CATEGORY_", 13) != 0 &&
            strncmp(&s[i], "CFG_Type_", 9) != 0)
        {
            memcpy(&t[j], &s[i-m], m);
            j += m;

            while (s[i] != ',' && s[i] != '>')
            {
                t[j++] = s[i++];
            }
            continue;
        }

        if (isdigit(s[i]) ||
            (s[i] == '-' && isdigit(s[i+1])))
        {
            memcpy(&t[j], &s[i-m], m);
            j += m;

            while (s[i] != ',' && s[i] != '>')
            {
                t[j++] = s[i++];
            }
            continue;
        }

        if (s[i] == '/' && s[i+1] == '*')
        {
            memcpy(&t[j], &s[i-m], m);
            j += m;

            i += 2;

            while (s[i] == ' ' || s[i] == '\t')
            {
                i += 1;
            }

            while (!(s[i] == '*' && s[i+1] == '/'))
            {
                t[j++] = s[i++];
            }

            while (t[j-1] == ' ' || t[j-1] == '\t')
            {
                j -= 1;
            }

            i += 2;
            continue;
        }

        while (s[i] != ',' && s[i] != '>')
        {
            i += 1;
        }

        if (s[i] == '>')
        {
            i += 1;

            while (s[i] != '\0')
            {
                t[j++] = s[i++];
            }
            break;
        }
    }

    t[j] = '\0';

    strcpy(line_comment, t);

    return j;
}


static int strip_header_file(char* hdr_data, int hdr_size)
{
    int  pos = 0;

    while (pos < hdr_size)
    {
        char   line_buf[1024];
        int    line_len = 0;
        char*  t;

        while (line_len < sizeof(line_buf) - 1)
        {
            char  ch;

            if (pos + line_len >= hdr_size)
                break;

            ch = hdr_data[pos + line_len];

            line_buf[line_len] = ch;
            line_len += 1;

            if (ch == '\n')
                break;
        }

        if (line_len == 0)
            break;

        line_buf[line_len] = '\0';

        t = strstr(line_buf, "//");

        if (t != NULL &&
            strchr(t, '<') != NULL &&
            strchr(t, '>') != NULL)
        {
            int  m = strlen(t);
            int  n = strip_cfg_comment(t);

            char*  p = hdr_data + pos + (int)(t - line_buf);

            memmove(p + n, p + m, hdr_size - (int)(p - hdr_data) - m);
            memcpy(p, t, n);

            hdr_size -= (m - n);
            line_len -= (m - n);
        }

        pos += line_len;
    }

    return hdr_size;
}


static int make_config_header(void)
{
    cfg_source_t*  p = &cfg_source;

    FILE*  hdr_file;
    char*  hdr_data;
    int    hdr_size;

    int    cfg_id = 0;
    int    i, j, n;

    static char  cfg_id_print_buf[256 * 64];
    int          cfg_id_print_len = 0;

    char   last_class_name[256] = "";

    if ((hdr_data = malloc(p->txt_size * 2 + 1)) == NULL)
        goto err;

    i = 0;
    j = 0;

    // -1: cfg_id would add 1 before it is used
    if (p->cfg_id_start > 0)
        cfg_id = p->cfg_id_start - 1;

    j = sprintf(&hdr_data[j],
        "\r\n"
        "#ifndef __CONFIG_H__\r\n"
        "#define __CONFIG_H__\r\n"
        "\r\n");

    while (i < p->txt_size)
    {
        char  class_name[256];

        int  n_pos, n_len;
        int  m_pos, m_len;

        if ((n = get_class(&p->txt_data[i], p->txt_size - i, &n_pos, &n_len, &m_pos, &m_len)) > 0)
        {
            char  upper_name[256];
            int   k;

            strncpy(class_name, &p->raw_data[i+n_pos], n_len);
            class_name[n_len] = '\0';

            for (k = 0; k <= n_len; k++)
            {
                upper_name[k] = toupper(class_name[k]);
            }

            if (cfg_id >= 0xFF)
            {
                printf("  failed: num_cfgs > 255\n");
                goto err;
            }

            if (strcmp(last_class_name, class_name) != 0)
            {
                char*  t = get_short_name(upper_name);

                strcpy(last_class_name, class_name);
                cfg_id += 1;

                cfg_id_print_len += sprintf
                (
                    &cfg_id_print_buf[cfg_id_print_len],
                    "#define CFG_ID_%s %*c0x%02X\r\n",
                    t,
                    (strlen(t) < 24) ? (24 - strlen(t)) : 1,
                    ' ',
                    cfg_id
                );
            }

            j += sprintf(&hdr_data[j], "typedef struct");

            k = m_pos - (n_pos + n_len);

            strncpy(&hdr_data[j], &p->raw_data[i+n_pos+n_len], k);
            j += k;

            j += make_members(&p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &hdr_data[j]);

            j += sprintf(&hdr_data[j], "\r\n} CFG_Struct_%s;", get_short_name(class_name));

            i += n;
            continue;
        }

        hdr_data[j++] = p->raw_data[i++];
    }

    j += sprintf(&hdr_data[j],
        "%s"
        "\r\n"
        "\r\n"
        "#endif  // __CONFIG_H__\r\n"
        "\r\n"
        "\r\n",
        cfg_id_print_buf);

    hdr_size = j;

    if (p->err_count > 0)
        goto err;

    if ((hdr_file = fopen(p->hdr_file, "wb")) == NULL)
        goto err;

    hdr_size = strip_header_file(hdr_data, hdr_size);

    fwrite(hdr_data, hdr_size, 1, hdr_file);

    fclose(hdr_file);

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}


static int make_config_source(void)
{
    cfg_source_t*  p = &cfg_source;

    FILE*  src_file;
    char*  src_data;
    int    src_size;

    int    cfg_id = 0;
    int    i, j, n;

    char   last_class_name[256] = "";

    if ((src_data = malloc(p->txt_size * 3 + 1)) == NULL)
        goto err;

    i = 0;
    j = 0;

    // -1: cfg_id would add 1 before it is used
    if (p->cfg_id_start > 0)
        cfg_id = p->cfg_id_start - 1;

    while (i < p->txt_size)
    {
        char  class_name[256];

        int  n_pos, n_len;
        int  m_pos, m_len;

        if ((n = get_class(&p->txt_data[i], p->txt_size - i, &n_pos, &n_len, &m_pos, &m_len)) > 0)
        {
            char  desc[256];
            int   desc_len;
            int   fixed_size = 0;
            int   k;

            desc_len = m_pos - (n_pos + n_len);

            if (desc_len > 0)
            {
                char*  t;

                strncpy(desc, &p->raw_data[i+n_pos+n_len], desc_len);
                desc[desc_len] = '\0';

                if ((t = strstr(desc, "fixed_size=")) != NULL)
                {
                    fixed_size = strtol(t + 11, NULL, 0);
                }
            }

            strncpy(class_name, &p->raw_data[i+n_pos], n_len);
            class_name[n_len] = '\0';

            j += sprintf(&src_data[j], "typedef struct");

            k = m_pos - (n_pos + n_len);

            strncpy(&src_data[j], &p->raw_data[i+n_pos+n_len], k);
            j += k;

            j += make_members(&p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &src_data[j]);

            j += sprintf(&src_data[j], "\r\n} CFG_Struct_%s;\r\n\r\n", get_short_name(class_name));

            if (strcmp(last_class_name, class_name) != 0)
            {
                strcpy(last_class_name, class_name);
                cfg_id += 1;
            }

            if (fixed_size > 0)
            {
                j += sprintf(&src_data[j], "CFG_Struct_%s  ID_%02X_%s_FS_%02X =\r\n{",
                    get_short_name(class_name), cfg_id, class_name, fixed_size);
            }
            else
            {
                j += sprintf(&src_data[j], "CFG_Struct_%s  ID_%02X_%s =\r\n{",
                    get_short_name(class_name), cfg_id, class_name);
            }

            j += make_assigns(&p->txt_data[i+m_pos], m_len, &p->raw_data[i+m_pos], &src_data[j]);

            j += sprintf(&src_data[j],
                "};");

            i += n;
            continue;
        }

        src_data[j++] = p->raw_data[i++];
    }

    src_size = j;

    if (p->err_count > 0)
        goto err;

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

    if (make_config_header() < 0)
        goto err;

    if (make_config_source() < 0)
        goto err;

    return 0;
err:
    return -1;
}


