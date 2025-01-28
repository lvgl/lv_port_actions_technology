#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/*
 * copy_cfg_info.exe ref_file  drc_file  out_file
 * 1. drc_file里有一些struct类型在ref_file里定义的，是直接使用类型名，注释里也有指定原型
 * 2. drc_file中变量赋值用的名称是ref_file里的enum类型值，在注释里面有指定原型
 *
 * 本程序作用
 * 1. 把这两点中相关的类型定义(enum和struct),从ref_file复制并合并到drc_file文件最前面,然后输出新文件。
 * 2. 固定包含typedef u8_t CFG_uint8;等、#include <types.h> 内容
 */

typedef struct
{
    char  *ref_file;
    char  *drc_file;
    char  *out_file;

    char  *drc_data;
    int    drc_fsize;

    char  *ref_data;
    int    ref_fsize;

} copy_cfg_t;

static copy_cfg_t  copy_cfg;


/* +  :第一个字符必须是指定的字符
 * *  :找到指定字符的数量（连续的），无则0个
 */
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

/* \\s+  :连续的空格制表符回车符换行符的总数量
 * \\w+  :连续的字母数字下划线的总数量（即标识符的名称长度）
 */
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


static int check_config_type(char* type)
{
    if (strncasecmp(type, "CFG_", 4) == 0)
        return 1;
    else
        return -1;
}


//必须用空格等空白符做分隔符
static int get_single_name(char* name, char* str_buf, int len)
{
    int i = 0, j = 0;

    j = str_match(&str_buf[i], len - i, "\\s*");

    i += j;

    if ((j = str_match(&str_buf[i], len - i, "\\w+")) < 0)
        return 0;

    if (name != NULL)
    {
        memcpy(name, &str_buf[i], j);
        name[j] = '\0';
    }

    i += j;
    return i;
}

static int add_new_name(char* names, char* str_buf, int len)
{
    char  temp[len + 1];
    char  *s = NULL;

    memcpy(temp, str_buf, len);
    temp[len] = '\0';

    //已有不添加
    s = strstr(names, temp);
    if (s && str_match(s, strlen(s), "\\w+") == len)
        return 0;

    //先添加分隔符
    if (strlen(names) > 0)
    {
        strcat(names, " ");
        len += 1;
    }

    strcat(names, temp);

    return len;
}


/* 处理单行注释
 * 检查注释里有没有类型定义名称，然后跳过这个注释
 */
static int check_single_annotation(char* text, int len, char* name, int* name_len)
{
    int i, j;

    if (!(text[0] == '/' && text[1] == '/'))
        return 0;
    
    i = 2;

    // 处理空格等字符后找到第一个'<'符号
    if ((j = str_match(&text[i], len - i, "\\s*<")) > 0)
        i += j;

    while (text[i] != '\0')
    {
        //没有完整处理掉换行符
        if (text[i] == '>' || text[i] == '\r' || text[i] == '\n')
        {
            i += 1;
            break;
        }

        if (strncmp(&text[i], "CFG_", 4) == 0)
        {
            j = str_match(&text[i], len -i, "\\w+");

            //class类型有指定归类, 需跳过
            if (strncmp(&text[i], "CFG_CATEGORY", 12) == 0)
            {
                i += j;
                continue;
            }

            if (name != NULL)
                *name_len += add_new_name(name, &text[i], j);

            i += j;
            continue;
        }
        i++;
    }
    return i;
}

static int get_enum_names(char* text, int len, char* tname, int* tlen)
{
    int  i, j, brackets;
    
    if (strncmp(&text[0], "enum", 4) != 0)
        return 0;

    i = 4;

    if ((j = str_match(&text[i], len - i, "\\s+")) <= 0)
        return 0;

    i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    if (tlen != NULL)
        *tlen += add_new_name(tname, &text[i], j);
    else
        add_new_name(tname, &text[i], j);

    i += j;

    /* 处理空格和换行符等 */
    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (text[i] != '{')
        return 0;

    i += 1;
    brackets = 1;
    
    /* 定位至相匹配的 '}'
     */
    while (i < len && brackets > 0)
    {
        if (text[i] == '{')
            brackets += 1;

        else if (text[i] == '}')
            brackets -= 1;

        i += 1;
    }

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    i += j;

    if (check_config_type(tname) < 0)
    {
        printf("  failed %s: config enum error: \"%s\"\n", __FUNCTION__, tname);

        exit(EXIT_FAILURE);
    }

    return i;
}

static int get_struct_names(char* text, int len, char* tname, int* tlen, char* sname, int* slen)
{
    int  i, j, brackets;
    
    if (strncmp(&text[0], "typedef struct", 14) != 0)
        return 0;

    i = 14;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if ((j = check_single_annotation(&text[i], len - i, sname, slen)) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (text[i] != '{')
        return 0;

    i += 1;
    brackets = 1;
    
    /* 定位至相匹配的 '}'
     */
    while (i < len && brackets > 0)
    {
        if (text[i] == '{')
            brackets += 1;
        else if (text[i] == '}')
            brackets -= 1;

        if ((j = check_single_annotation(&text[i], len - i, sname, slen)) > 0)
        {
            i += j;
            continue;
        }

        i += 1;
    }

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    if (tlen != NULL)
        *tlen += add_new_name(tname, &text[i], j);
    else
        add_new_name(tname, &text[i], j);

    i += j;

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    i += j;

    if (check_config_type(tname) < 0)
    {
        printf("  failed %s: config struct error: \"%s\"\n", __FUNCTION__, tname);
        
        exit(EXIT_FAILURE);
    }

    return i;
}

static int get_class_names(char* text, int len, char* sname, int* slen)
{
    int i, j, brackets;

    if (strncmp(&text[0], "class", 5) != 0)
        return 0;

    i = 5;

    if ((j = str_match(&text[i], len - i, "\\s+")) <= 0)
            return 0;
    
    i += j;

    if ((j = str_match(&text[i], len - i, "\\w+")) <= 0)
        return 0;

    if (check_config_type(&text[i]) < 0)
        return 0;

    i += j;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if ((j = check_single_annotation(&text[i], len - i, sname, slen)) > 0)
        i += j;

    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (text[i] != '{')
        return 0;

    i += 1;
    brackets = 1;

    while (text[i] != '\0' && brackets > 0)
    {
        if (text[i] == '{')
            brackets += 1;
        else if (text[i] == '}')
            brackets -= 1;

        if ((j = check_single_annotation(&text[i], len - i, sname, slen)) > 0)
            i += j;

        i++;
    }

    if ((j = str_match(&text[i], len - i, "\\s*;")) <= 0)
        return 0;

    i += j;

    return i;
}


static int get_args(int argc, char* argv[])
{
    copy_cfg_t*  p = &copy_cfg;

    if (argc != 4)
        goto err;

    p->ref_file = argv[1];
    p->drc_file = argv[2];
    p->out_file = argv[3];

    return 0;
err:
    printf("usage:\n\tcopy_cfg_info.exe ref_file drc_file out_file\n");
    return -1;
}

static int read_config_file(void)
{
    copy_cfg_t*  p = &copy_cfg;

    if ((p->ref_data = load_file(p->ref_file, &p->ref_fsize)) == NULL)
        goto err;

    if ((p->drc_data = load_file(p->drc_file, &p->drc_fsize)) == NULL)
            goto err;

    return 0;
err:
    printf("  failed %s\n", __FUNCTION__);

    return -1;
}

static int is_in_str(char* s1, char* s2)
{
    char* s = NULL;
    int len = strlen(s2);

    s = strstr(s1, s2);
    if (s && str_match(s, strlen(s), "\\w+") == len)
        return 1;

    return 0;
}

/* 拿到配置项名称，从大堆数据里找，相同名称则查看它的注释，注释里有其他配置项则递归进去
 */
static int copy_cfg_info(char* text, int len, char* tname, char* cfg_name, char* data_buf)
{
    char find_name[64];
    int i, j, n;

    i = j = 0;

    while(i < len)
    {
        memset(find_name, 0, 64);
        n = 0;

        if ((j = get_enum_names(&text[i], len - i, find_name, &n)) > 0)
        {
            //找到了对应的enum类型
            if (strcmp(cfg_name, find_name) == 0)
            {
                add_new_name(tname, find_name, n);

                memcpy(data_buf, &text[i], j);
                data_buf += j;
                memcpy(data_buf, "\r\n\r\n", 4);

                return j + 4;
            }

            i += j;
            continue;
        }

        if ((j = get_struct_names(&text[i], len - i, find_name, &n, NULL, NULL)) > 0)
        {
            if (strcmp(cfg_name, find_name) == 0)
            {
                int k, m = 0, brackets = 0, pos = i;

                add_new_name(tname, find_name, n);

                for (pos = i; (text[pos] != '{') && (pos < i + j); pos++);

                if (text[pos] == '{')
                    brackets = 1;


                //检查注释,深层的要数据要放在前面,否则编译不过
                while (pos < i + j && brackets > 0)
                {
                    memset(find_name, 0, 64);
                    n = 0;

                    //处理一行注释，如有其他名称则递归
                    if ((k = check_single_annotation(&text[pos], j - pos, find_name, &n)) > 0)
                    {
                        //若没有其他名称, n = 0
                        if (n > 0 && !is_in_str(tname, find_name)) {
                            m += copy_cfg_info(&text[0], i, tname, find_name, &data_buf[m]);
                        }
                        pos += k;
                        continue;
                    }

                    if (text[pos] == '{')
                        brackets += 1;
                    else if (text[pos] == '}')
                        brackets -= 1;

                    pos++;
                }

                memcpy(&data_buf[m], &text[i], j);
                m += j;
                memcpy(&data_buf[m], "\r\n\r\n", 4);

                return m + 4;
            }            
        }
        i++;
    }

    return 0;
}


static int make_file(void)
{
    copy_cfg_t*  p = &copy_cfg;

    FILE *out_file;

    int   i, j, n;
    int   tn_len, sn_len;

    char *out_data  = malloc(1*1024*1024);
    char *type_names = malloc(2*1024);
    char *sub_names  = malloc(1*1024);

    if (type_names == NULL || sub_names == NULL || out_data == NULL)
        goto err;

    i = j = n = 0;
    tn_len = sn_len = 0;

    //每次直接在后面添加新字符串，因此必须初始化为空，否则可能会使用了垃圾值
    memset(type_names, 0, 2*1024);
    memset(sub_names, 0, 1*1024);

    // 找到拓展文件中所有类型并获取它们的名字
    while (i < p->drc_fsize)
    {
        if ((n = get_enum_names(&p->drc_data[i], p->drc_fsize - i, type_names, &tn_len)) > 0)
        {
            i += n;
            continue;
        }

        if ((n = get_struct_names(&p->drc_data[i], p->drc_fsize - i, type_names, &tn_len, sub_names, &sn_len)) > 0)
        {
            i += n;
            continue;
        }

        if ((n = get_class_names(&p->drc_data[i], p->drc_fsize - i, sub_names, &sn_len)) > 0)
        {
            i += n;
            continue;
        }

        i++;
    }

    // 固定添加信息
    j = sprintf(out_data,
            "//EXTRA_ADDED_START\r\n"
            "\r\n"
            "#include <types.h>\r\n"
            "\r\n"
            "typedef u8_t    cfg_uint8;\r\n"
            "typedef u16_t   cfg_uint16;\r\n"
            "typedef u32_t   cfg_uint32;\r\n"
            "\r\n"
            "typedef s8_t    cfg_int8;\r\n"
            "typedef s16_t   cfg_int16;\r\n"
            "typedef s32_t   cfg_int32;\r\n"
            "\r\n");

    i = 0;

    // 从ref_file里复制出来
    while (i < sn_len && sub_names[i] != '\0')
    {
        char temp_name[64];

        memset(temp_name, 0, 64);
        if ((n = get_single_name(temp_name, &sub_names[i], sn_len - i)) > 0)
        {
            if (is_in_str(type_names, temp_name))
            {
                i += n;
                continue;
            }

            j += copy_cfg_info(p->ref_data, p->ref_fsize, type_names, temp_name, &out_data[j]);
            i += n;
        }
        i++;
    }

    j += sprintf(&out_data[j],
            "//EXTRA_ADDED_END\r\n"
            "\r\n");

    out_file = fopen(p->out_file, "wb");
    if(out_file == NULL)
        goto err;

    fwrite(out_data, 1, j, out_file);
    fwrite(p->drc_data, 1, p->drc_fsize, out_file);

    fclose(out_file);

    free(type_names);
    free(sub_names);
    free(out_data);

    out_file = NULL;
    type_names = NULL;
    sub_names = NULL;
    out_data = NULL;

    return 0;

err:
    if (type_names)
        free(type_names);
    if (sub_names)
        free(sub_names);
    if (out_data)
        free(out_data);

    type_names = NULL;
    sub_names = NULL;
    out_data = NULL;

    return -1;
}


int main(int argc, char* argv[])
{
    if (get_args(argc, argv) < 0)
        goto err;

    if (read_config_file() < 0)
        goto err;

    if (make_file() < 0)
        goto err;

    return 0;

err:
    printf("fail\n");
    return -1;
}
