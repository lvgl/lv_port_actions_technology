/*
 * xml_cleaner.c 
 * xml文件清理程序，需要指定至少1组关键词kw及其kwstr
 * 作用方式：
 *  -d    若标签名"name"包含关键词，则删除整个标签。
 *  -k    若标签名"name"包含关键词则保留，否则删除整个标签。
 *  -a    在文件未尾追加信息，kwstr是包含追加信息的文件。"-a null"是保留的，用于追加自定义音效的config。
 *  -h    如果标签是enum, struct, config类型，则保留，否则删除。
 *  -x    删除某个symbol及其值。此关键词仅对config类型的标签起作用，对item不起作用。
 *  -i    删除某个symbol及其值。次关键词仅对enum和struct类型的item起作用。
 *
 *  xml_cleaner.exe xml_file[ out_xmlfile] kw1 kwstr1 [kw2 kwstr2 ...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define YES  1
#define NO   0


#define KWSTR_LEN   (0x10)


//define all your keywords here
static const char KeyWords[] = {'d', 'h', 'x', 'i', 'k', 'a'};


typedef struct
{
    char  *xmlfile;
    char  *outfile;

    char  *xmldata;
    int    xmlsize;

    int    kwnums;
    char **kwstr;    // kwstr[i] points to a kwstr inputted
    char   kw[0];    // variable length array(malloc)

} xml_clean_t;

/* malloc before used and free after used */
xml_clean_t  *xmlc;


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


#define CUSTOM_DAE \
("<config name=\"BTMusic_Multi_Dae_Custom\" title=\"自定义音效\" \
cfg_id=\"0xF0\" size=\"142\" num_items=\"2\" category=\"ASET\" attr=\"adjust_online\">\r\n\
\t<item type=\"Type_DAE_Settings\" name=\"Dae_Settings\" title=\"音效参数\" offs=\"0\"   \
size=\"126\" refer=\"Type_DAE_Settings\" attr=\"click_popup\" />\r\n\
\t<item type=\"uint8\"             name=\"Name\"         title=\"音效名称\" \
offs=\"126\" size=\"16\"  range=\"\"                  array=\"16\" attr=\"string\" />\r\n\
</config>\r\n\r\n")


typedef enum
{
    KWS_LOGIC_OR = 0,
    KWS_LOGIC_AND,

} kws_logic_e;


static int get_file_size(char *fname)
{
    FILE *file = NULL;
    int size = 0;

    if ((file = fopen(fname, "rb")) == NULL)
        return 0;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);

    return size;
}

static int load_file(char *fname, char *buf)
{
    FILE *file = NULL;
    int   fsize = 0;

    if ((file = fopen(fname, "rb")) == NULL)
    {
        printf("load %s fail\n", fname);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fread(buf, 1, fsize, file);
    
    fclose(file);
    file = NULL;

    return fsize;
}

static inline char *realloc_safe(char *ptr, int size)
{
    char *temp = realloc(ptr, size);

    if(!temp)
        free(ptr);

    return temp;
} 

static inline int count_to_end(char *str, char c)
{
    int j = 0;

    while(str[j] != c && str[j] != '\0')
        j++;

    return j;
}

/* only replace a char once
 */
static inline void str_replace_c(char *str, char find, char replace)
{
    char *c = str;

    while(*c)
    {
        if(*c == find)
            *c = replace;
        c++;
    }
}

static inline int str_count_c(char *str, char find)
{
    int i = 0;
    char *c = str;

    while(*c)
    {
        if (*c == find)
            i++;
        c++;
    }

    return i;
}

/* split str into "num" strings, and "ptr[i]" points to each of them
 */
static void str_split_c(char *str, char find, char *ptr[], int num)
{
    int i = 0;
    char *c = str;

    if (num <= 0)
        return ;

    ptr[i++] = str;

    while(*c)
    {
        if (*c == find)
        {
            *c = '\0';
            ptr[i++] = c + 1;
        }

        if(i == num)
            break;

        c++;
    }
}

static int check_kw_valid(char c)
{
    int i, n;
    n = sizeof(KeyWords) / sizeof(KeyWords[0]);

    for (i = 0; i < n; i++)
    {
        if (c == KeyWords[i])
            return YES;
    }

    return NO;
}

static int check_kwstr_logic(char *name, char *kws[], int num, kws_logic_e mode)
{
    int i;

    if (num == 0 || name == NULL)
        return NO;

    for (i = 0; i < num; i++)
    {
        if (strstr(name, kws[i]))
        {
            if (mode == KWS_LOGIC_OR)
                return YES;
        }
        else
        {
            if (mode == KWS_LOGIC_AND)
                return NO;
        }
    }

    if (mode == KWS_LOGIC_AND)
        return YES;

    return NO;
}

static int check_is_xml_file(char *name)
{
    int len = strlen(name);

    if (strcmp(&name[len - 4], ".xml") == 0)
        return YES;

    return NO;
}

static int get_tag(char *text, int len, char *name, char *start, char *end)
{
    int i, j;

    if (strncmp(&text[0], start, strlen(start)) != 0)
        return 0;

    i = strlen(start);

    // there is at least 1 space or blank
    if ((j = str_match(&text[i], len - i, "\\s+")) < 0)
        return 0;

    i += j;

    if (name)
    {
        if (strncmp(&text[i], "name=", 5) != 0)
            return 0;

        i += 5;

        if (text[i] != '\"')
            return 0;

        i += 1;

        if ((j = str_match(&text[i], len -i, "\\w+")) < 0)
            return 0;

        memcpy(name, &text[i], j);
        name[j] = '\0';
        i += j;

        if ((j = str_match(&text[i], len - i, "\\s*\"")) < 0)
            return 0;
        
        i += j;
    }

    while(i < len)
    {
        if (strncmp(&text[i], end, strlen(end)) == 0)
        {
            i += strlen(end);

            // deal with blanks, otherwise there would be new blank lines after deleted
            if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
                i += j;

            break;
        }

        i++;
    }

    return i;
}

/* text[*s_pos] is the beginning of symbol, while *s_len shows its length.
 * Return checked length as soon as one symbol found.
 */
static int get_symbol(char *text, int len, char *symbol, int *s_pos, int *s_len)
{
    int i = 0, j = 0, n;

    if (!symbol)
        return 0;

    n = strlen(symbol);

    // 3 = strlen(="")
    if (len < n + 3)
        return 0;

    // locate to the start of symbol
    while(i + n+3 < len)
    {
        if (strncmp(&text[i], symbol, n) == 0)
            break;

        i++;
    }

    if (i +n+3 >= len)
        return 0;

    if (s_pos)
        *s_pos = i;

    i += n;

    // spaces around '=' ?
    if ((j = str_match(&text[i], len - i, "\\s*=\\s*\"")) < 0)
        return 0;

    i += j;

    // match the corresponding '\"'
    while(i < len)
    {
        if (text[i] == '\"')
        {
            i += 1;
            break;
        }
        i++;
    }

    // deal with spaces, otherwise there would be new spaces after deleted
    if ((j = str_match(&text[i], len - i, "\\s*")) > 0)
        i += j;

    if (s_len)
        *s_len = i - *s_pos;

    return i;
}

/* Delete all the symbol in given "text" with "condition" checked ok,
 * so you'd better use a small range of "text" input.
 * "condition" is optional, "symbol"(and its value) will be deleted directly without "condition".
 */
static int delete_symbol(char *text, int len, char *ndata, char *symbol, char *condition)
{
    int i, cdt;
    int s_pos, s_len, nsize;

    if (!symbol)
        return 0;

    if (condition)
    {
        i = 0;
        cdt = 0;

        while(i < len)
        {
            if (!strncmp(&text[i], condition, strlen(condition)))
            {
                cdt = 1;
                break;
            }
            i++;
        }

        if (cdt == 0)
        {
            memcpy(ndata, text, len);
            return len;
        }
    }

    i = 0;
    nsize = 0;

    while(i < len)
    {
        s_pos = 0;
        s_len = 0;

        if (get_symbol(&text[i], len - i, symbol, &s_pos, &s_len) > 0)
        {
            memcpy(&ndata[nsize], &text[i], s_pos);
            nsize += s_pos;
            i += s_pos + s_len;
            continue;
        }
        else
        {    //no symbol anymore, just copy data and break
            memcpy(&ndata[nsize], &text[i], len - i);
            nsize += len - i;
            break;
        }
    }

    return nsize;
}


static int xml_get_enum(char *text, int len, char *name)
{
    return get_tag(text, len, name, "<enum", "</enum>");
}


static int xml_get_struct(char *text, int len, char *name)
{
    return get_tag(text, len, name, "<struct", "</struct>");
}


static int xml_get_config(char *text, int len, char *name)
{
    return get_tag(text, len, name, "<config", "</config>");
}


/* -d kwstr
 * delete the tags that contain kwstr
 */
static int xmlc_delete_tag(char *ndata, char *kwstr, int ks_nums)
{
    int i, j, nsize, istag;
    char tagname[64];
    char *kws[ks_nums];

    str_split_c(kwstr, ',', kws, ks_nums);

    i = nsize = 0;

    while(i < xmlc->xmlsize)
    {
        istag = 0;

        if ((j = xml_get_enum(&xmlc->xmldata[i], xmlc->xmlsize - i, tagname)) > 0)
            istag = 1;
        else if ((j = xml_get_struct(&xmlc->xmldata[i], xmlc->xmlsize - i, tagname)) > 0)
            istag = 1;
        else if ((j = xml_get_config(&xmlc->xmldata[i], xmlc->xmlsize - i, tagname)) > 0)
            istag = 1;

        if (istag)
        {
            if (!check_kwstr_logic(tagname, kws, ks_nums, KWS_LOGIC_AND))
            {
                memcpy(&ndata[nsize], &xmlc->xmldata[i], j);
                nsize += j;
            }

            i += j;
            continue;
        }

        ndata[nsize++] = xmlc->xmldata[i++];
    }

    return nsize;
}


/* -h
 * Delete the tags that are NOT one of enum, struct or config.
 */
static int xmlc_delete_misc(char *ndata)
{
    int i, j, nsize, istag;

    i = nsize = 0;

    while(i < xmlc->xmlsize)
    {
        istag = 0;

        if ((j = xml_get_enum(&xmlc->xmldata[i], xmlc->xmlsize - i, NULL)) > 0)
            istag = 1;
        else if ((j = xml_get_struct(&xmlc->xmldata[i], xmlc->xmlsize - i, NULL)) > 0)
            istag = 1;
        else if ((j = xml_get_config(&xmlc->xmldata[i], xmlc->xmlsize - i, NULL)) > 0)
            istag = 1;

        if (istag)
        {
            memcpy(&ndata[nsize], &xmlc->xmldata[i], j);
            nsize += j;

            i += j;
            continue;
        }

        i++;
    }

    return nsize;
}


/* -x kwstr
 * <config ...kwstr="*"... > 
 * Delete the string (kwstr="*") as above, without doing effect to its item.
 * It only works for "config" tag and would NOT delete same symbol in "enum" or "struct".
 */
static int xmlc_delete_config_symbol(char *ndata, char *kwstr, int ks_nums)
{
    int i, j, nsize;
    char *kws[ks_nums];

    // only 1 symbol could be deleted once?
    if (ks_nums != 1){
        printf("[E] -x only receive 1 kwstr\n");
        return -1;
    }

    str_split_c(kwstr, ',', kws, ks_nums);

    i = nsize = 0;

    while(i < xmlc->xmlsize)
    {
        if ((j = get_tag(&xmlc->xmldata[i], xmlc->xmlsize - i, NULL, "<config", ">")) > 0)
        {
            nsize += delete_symbol(&xmlc->xmldata[i], j, &ndata[nsize], kwstr, NULL);
            i += j;
            continue;
        }

        ndata[nsize++] = xmlc->xmldata[i++];
    }

    return nsize;
}


/* -i kwstr
 * <item ...symbol="*"...cdt="*"... /> 
 * Delete the string (kwstr="*") as above in item of "struct" and "config".
 * cdt(condition) is just the symbol name, not caring its value and it is optional.
 * cdt is optional, symbol(and its valude) will be deleted directly without cdt.
 */
static int xmlc_delete_item_symbol(char *ndata, char *kwstr, int ks_nums)
{
    int i, j, n;
    int nsize, istag;
    char *data = xmlc->xmldata;
    char *cdt = NULL;
    char *kws[ks_nums];

    if (ks_nums != 1 && ks_nums != 2)
        return -1;

    str_split_c(kwstr, ',', kws, ks_nums);

    if (ks_nums == 2)
        cdt = kws[1];

    i = nsize = 0;

    while(i < xmlc->xmlsize)
    {
        istag = 0;

        if ((j = xml_get_struct(&data[i], xmlc->xmlsize - i, NULL)) > 0)
            istag = 1;
        else if ((j = xml_get_config(&data[i], xmlc->xmlsize - i, NULL)) > 0)
            istag = 1;

        if (istag)
        {
            n = i;
            while(n < xmlc->xmlsize && j > 0)
            {
                if(!strncmp(&data[n], "<item", strlen("<item")))
                    break;
                n++;
                j--;
            }
            memcpy(&ndata[nsize], &data[i], n - i);
            nsize += n - i;
            i = n;

            while(j > 0)
            {
                // it's supposed to get item every time
                if ((n = get_tag(&data[i], j, NULL, "<item", "/>")) > 0)
                {
                    nsize += delete_symbol(&data[i], n, &ndata[nsize], kws[0], cdt);
                    i += n;
                    j -= n;
                }
                else
                {
                    memcpy(&ndata[nsize], &data[i], j);
                    nsize += j;
                    i += j;
                    break;
                }
            }

            continue;
        }

        ndata[nsize++] = xmlc->xmldata[i++];
    }

    return nsize;
}


/* -k kwstr
 * Only remain the tags that contain kwstr, and who don't would be deleted.
 */
static int xmlc_keep_tag(char *ndata, char *kwstr, int ks_nums)
{
    int i, j, nsize, istag;
    char tagname[64];
    char *kws[ks_nums];

    str_split_c(kwstr, ',', kws, ks_nums);

    i = nsize = 0;

    while(i < xmlc->xmlsize)
    {
        istag = 0;

        if ((j = xml_get_enum(&xmlc->xmldata[i], xmlc->xmlsize - i, tagname)) > 0)
            istag = 1;
        else if ((j = xml_get_struct(&xmlc->xmldata[i], xmlc->xmlsize - i, tagname)) > 0)
            istag = 1;
        else if ((j = xml_get_config(&xmlc->xmldata[i], xmlc->xmlsize - i, tagname)) > 0)
            istag = 1;

        if (istag)
        {
            if (check_kwstr_logic(tagname, kws, ks_nums, KWS_LOGIC_OR))
            {
                memcpy(&ndata[nsize], &xmlc->xmldata[i], j);
                nsize += j;
            }

            i += j;
            continue;
        }

        ndata[nsize++] = xmlc->xmldata[i++];
    }

    return nsize;
}


/* -a kwstr
 * Append rawdata to xmlfile, expect a file input, but "-a null" is reserved for custom dae.
 * it would finish other append even though some append fail, and return new-size appended successfully,
 * while return -1 if all append fail.
 */
static int xmlc_append_tag(char *ndata, char *kwfiles, int ks_nums)
{
    int i, asize;
    int hastail = 0, nsize = 0;
    char *endtag = NULL;
    char *kws[ks_nums];

    str_split_c(kwfiles, ',', kws, ks_nums);

    memcpy(ndata, xmlc->xmldata, xmlc->xmlsize);
    nsize = xmlc->xmlsize;
    i = nsize;

    while (i >= 0)
    {
        /* Append in the front if there is "</config_file>" at the tail.
         * Break as soon as '<' is checked.
         */
        if (ndata[i] == '<')
        {
            if (strncmp(&ndata[i], "</config_file>", strlen("</config_file>")) == 0)
            {
                endtag = malloc(nsize - i + 1);
                if (!endtag)
                    break;

                memset(endtag, 0, nsize - i);
                memcpy(endtag, &ndata[i], nsize - i);
                endtag[nsize - i] = '\0';

                memset(&ndata[i], 0, nsize - i);
                nsize = i;
                hastail = 1;
            }

            break;
        }

        i--;
    }

    for (i = 0; i < ks_nums; i++)
    {
        if (strcmp(kws[i], "null") == 0
         || strcmp(kws[i], "NULL") == 0) 
        {
            //append custom dae
            nsize += sprintf(&ndata[nsize], CUSTOM_DAE);
        }
        else
        {
            // only xml file could be appended?
            if (!check_is_xml_file(kws[i]))
                continue;

            asize = load_file(kws[i], &ndata[nsize]);
            if (asize > 0)
                nsize += asize;
        }
    }

    if (hastail)
    {
        nsize += sprintf(&ndata[nsize], endtag);
        free(endtag);
        endtag = NULL;
    }

    // nsize = xmlc->xmlsize if all append fail
    return (nsize == xmlc->xmlsize) ? -1 : nsize;
}


/* calc the extra size to be appended
 */
static int calc_append_size()
{
    int i, j, n = 0; 
    int anum = 0, asize = 0;
    char *akwstr = NULL;
    char aname[0x20 + 1];

    for(i = 0; i < xmlc->kwnums; i++)
        if (xmlc->kw[i] == 'a')
            break;

    if (i == xmlc->kwnums)
        return 0;

    // "-a" without input, default custom dae
    if (xmlc->kwstr[i][0] == '\0')
        strcpy(xmlc->kwstr[i], "null");

    akwstr = xmlc->kwstr[i];

    anum = str_count_c(akwstr, ',') + 1;
    for (i = 0; i < anum; i++)
    {
        memset(aname, 0, 0x20 + 1);

        if ((j = count_to_end(&akwstr[n], ',')) <= 0)
            break;

        // file name is too long
        if (j > 0x20)
        {
            printf("append fail: file name longer than 0x20\n");
            return -1;
        }

        memcpy(aname, &akwstr[n], j);
        aname[j] = '\0';

        // +1: to skip separator ','
        n += j + 1;

        // Reserved for custom dae
        if (strcmp(aname, "null") == 0
         || strcmp(aname, "NULL") == 0)
        {
            // +1: for EOF
            asize += strlen(CUSTOM_DAE) + 1;
            continue;
        }

        // only xml file could be append?
        if (!check_is_xml_file(aname))
        {
            printf("append fail: %s is not xml file\n", aname);
            return -1;
        }

        asize += get_file_size(aname);
    }

    return asize;
}


static void do_clean(void)
{
    int i = 0, kwstr_nums;
    int nsize, asize;
    char *ndata = NULL;
    FILE *outfile = NULL;

    if (xmlc->xmlsize <= 0)
        return ;

    asize = calc_append_size();
    
    // +2: 1byte to avoid problem caused by asize = -1
    //   : 1byte to place a guard '\0'(0x00)
    nsize = xmlc->xmlsize + asize + 2;

    ndata = malloc(nsize);
    xmlc->xmldata = malloc(nsize);

    if (!ndata || !xmlc->xmldata)
        goto end;

    memset(xmlc->xmldata, 0, nsize);

    if (load_file(xmlc->xmlfile, xmlc->xmldata) <= 0)
        goto end;

    while(i < xmlc->kwnums)
    {
        // pre-clean the data for next process
        memset(ndata, 0, nsize);
        nsize = 0;

        // count the kwstr nums
        kwstr_nums = str_count_c(xmlc->kwstr[i], ',') + 1;
        if (xmlc->kwstr[i][0] == '\0')
            kwstr_nums = 0;

        switch (xmlc->kw[i])
        {
        case 'd':
            nsize = xmlc_delete_tag(ndata, xmlc->kwstr[i], kwstr_nums);
            break;

        case 'h':
            nsize = xmlc_delete_misc(ndata);
            break;

        case 'x':
            nsize = xmlc_delete_config_symbol(ndata, xmlc->kwstr[i], kwstr_nums);
            break;

        case 'i':
            nsize = xmlc_delete_item_symbol(ndata, xmlc->kwstr[i], kwstr_nums);
            break;

        case 'k':
            nsize = xmlc_keep_tag(ndata, xmlc->kwstr[i], kwstr_nums);
            break;

        case 'a':
            // append file error: not xml file or file name too long?
            if (asize < 0)
            {
                nsize = -1;
                break;
            }

            nsize = xmlc_append_tag(ndata, xmlc->kwstr[i], kwstr_nums);
            break;

        default :
            nsize = -1;
            break;
        }

        // updata new data for next process
        if (nsize >= 0 && nsize != xmlc->xmlsize)
        {
            memset(xmlc->xmldata, 0, xmlc->xmlsize);
            memcpy(xmlc->xmldata, ndata, nsize);
            xmlc->xmlsize = nsize;
        }
        i++;
    }

    if ((outfile = fopen(xmlc->outfile, "wb")) == NULL)
        goto end;

    fwrite(xmlc->xmldata, xmlc->xmlsize, 1, outfile);

    fclose(outfile);
    outfile = NULL;
    
end:
    if (ndata)
    {
        free(ndata);
        ndata = NULL;
    }

    if (xmlc->xmldata)
    {
        free(xmlc->xmldata);
        xmlc->xmldata = NULL;
    }
}


static void memory_release(void)
{
    int i;

    for (i = 0; i < xmlc->kwnums; i++)
    {
        if (xmlc->kwstr[i])
            free(xmlc->kwstr[i]);
        xmlc->kwstr[i] = NULL;
    }

    if (xmlc->kwstr)
    {
        free(xmlc->kwstr);
        xmlc->kwstr = NULL;
    }

    if (xmlc->xmldata)
    {
        free(xmlc->xmldata);
        xmlc->xmldata = NULL;
    }

    free(xmlc);
    xmlc = NULL;
}


/* eg: xml_cleaner.exe src.xml[ dst.xml] -h -i sym[ cdt] -k "TM_"
 */
static int read_args(int argc, char *argv[])
{
    int i, iskw, nums = 0;

    // need 3 args at least, eg: xml_cleaner.exe src.xml -a
    if ((argc < 3) || (argc == 3 && argv[2][0] != '-'))
        goto err_args2;

    // argv[2] or argv[3] must be kw who is with '-'
    if ((argv[2][0] != '-')
      && (argc >= 4 && argv[3][0] != '-'))
      goto err_args2;

    for (i = 2; i < argc; i++)
        if (argv[i][0] == '-')
            nums++;

    // nums * 1 = sizeof(xmlc->kw)
    if (!(xmlc = malloc(sizeof(xml_clean_t) + nums * 1)))
        goto err_args2;

    memset(xmlc, 0, sizeof(xml_clean_t) + nums * 1);
    xmlc->kwstr = malloc(sizeof(xmlc->kwstr[0]) * nums);

    xmlc->xmlfile = argv[1];
    xmlc->outfile = argv[1];
    
    // output file specified?
    if (argv[2][0] != '-')
        xmlc->outfile = argv[2];

    xmlc->kwnums = nums;

    if (!check_is_xml_file(xmlc->xmlfile))
        goto err_args1;

    if (!check_is_xml_file(xmlc->outfile))
        goto err_args1;

    nums = 0;
    iskw = 0;
    for (i = 2; i < argc; i++)
    {
        if (!strcmp(argv[i], xmlc->outfile))
            continue;

        if (argv[i][0] == '-')
        {
            if (nums >= xmlc->kwnums)
                goto err_args1;

            xmlc->kw[nums] = argv[i][1];

            if (!check_kw_valid(xmlc->kw[nums]))
                goto err_args1;

            //pre-malloc for kwstr to avoid null pointer
            xmlc->kwstr[nums] = malloc(KWSTR_LEN);
            memset(xmlc->kwstr[nums], 0, KWSTR_LEN);
            iskw = 1;
            nums++;
        }
        else
        {
            // nums - 1: make sure kwstr is the same index as xmlc->kw
            char *temp = xmlc->kwstr[nums - 1];
            int len = strlen(temp);

            // another 1byte for EOF
            len += strlen(argv[i]) + 1;

            // 2nd or more kwstr for a kw, another 1byte for ','
            if (!iskw)
                len += 1;

            if (len > KWSTR_LEN)
            {
                temp = realloc_safe(temp, len);
                xmlc->kwstr[nums - 1] = temp;
            }

            if (temp)
            {
                /* replace blank to KWSTR_SEP */
                str_replace_c(argv[i], ' ', ',');

                if(!iskw)
                    strcat(temp, ",");

                strcat(temp, argv[i]);
            }

            iskw = 0;
        }
    }

    xmlc->kwnums = nums;
    return 0;

err_args1:
    memory_release();

err_args2:
    printf("\nusage:  xml_cleaner.exe xml_file[ out_xmlfile] kw1 kws1 ... [kw2 kws2 ...]\n");
    printf("Content in [] is optional. Each kw can receive mulitple input unless stated itself\n");
    printf("\t-d kws ...   : delete the whole tag whose \"name\" does contain kws\n");
    printf("\t-k kws ...   : only keep tags whose \"name\" does contain kws\n");
    printf("\t-a kws ...   : append all raw data to xml_file, \"-a null\" is reserved for custom dae\n");
    printf("\t-x kws       : only 1 symbol, delete symbol in config tag header and do no effect to its items\n");
    printf("\t-i kws[ cdt] : no more than 2 inputs, delete symbol in items of struct and config, cdt is optional\n");
    printf("\t-h           : no kws! delete the tags that are NOT one of enum, struct or config\n");
    return -1;
}


int main(int argc, char *argv[])
{
    if (read_args(argc, argv) < 0)
        return -1;

    if ((xmlc->xmlsize = get_file_size(xmlc->xmlfile)) <= 0)
        goto err;

    do_clean();

    memory_release();

    return 0;

err:
    printf("fail\n");
    memory_release();
    return -1;
}
