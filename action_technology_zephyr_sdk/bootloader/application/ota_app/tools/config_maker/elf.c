/*
 * elf.c
 */

typedef unsigned char   u8_t;
typedef unsigned short  u16_t;
typedef unsigned int    u32_t;

typedef signed char   s8_t;
typedef signed short  s16_t;
typedef signed int    s32_t;


typedef u32_t  Elf_Addr;   // 无符号地址
typedef u16_t  Elf_Half;   // 无符号半整型
typedef u32_t  Elf_Off;    // 无符号文件偏移量
typedef s32_t  Elf_Sword;  // 有符号整型
typedef u32_t  Elf_Word;   // 无符号整型
typedef u8_t   Elf_Uchar;  // 无符号短整型


#define ELFMAG         "\x7f""ELF"  // ELF 魔数

#define EI_NIDENT      16
#define ET_REL         1  // 可重定位文件
#define ET_EXEC        2  // 可执行文件
#define EM_MIPS        8  // MIPS 体系结构
#define EF_MIPS_PIC    2  // MIPS 位置无关代码
#define EF_MIPS_CPIC   4  // MIPS 位置无关代码
#define PT_LOAD        1  // 可装载段
#define PF_X           1  // 可执行

#define SHT_NULL       0  // 无效节头
#define SHT_PROGBITS   1  // 程序
#define SHT_SYMTAB     2  // 符号表
#define SHT_STRTAB     3  // 字符串表
#define SHT_RELA       4  // 重定位节
#define SHT_NOBITS     8  // 空内容
#define SHT_REL        9  // 重定位节

#define SHF_ALLOC      2  // 分配内存
#define SHF_EXECINSTR  4  // 指令代码

#define STB_LOCAL      0  // 局部符号
#define STB_GLOBAL     1  // 全局符号
#define STB_WEAK       2  // 弱符号

#define STT_OBJECT     1  // 数据对象
#define STT_FUNC       2  // 函数
#define STT_SECTION    3  // 节
#define STT_FILE       4  // 文件

#define STV_DEFAULT    0  // 默认可见性
#define STV_INTERNAL   1  // 内部
#define STV_HIDDEN     2  // 隐藏
#define STV_PROTECTED  3  // 保护

#define STO_MIPS16     0xf0    // MIPS16

#define SHN_UNDEF      0       // 未定义
#define SHN_ABS        0xfff1  // 绝对值


#define ELF_ST_BIND(_i)  ((_i) >> 4)   // 符号绑定
#define ELF_ST_TYPE(_i)  ((_i) & 0xf)  // 符号类型

#define ELF_ST_INFO(_b, _t)  \
    \
    (((_b) << 4) | ((_t) & 0xf))


#define ELF_ST_VISIBILITY(_o)  ((_o) & 0x3)  // 可见性

#define SET_ST_VISIBILITY(_o, _v)  \
    \
    (_o) = (((_o) & ~0x3) | (_v))


#define ELF_ST_IS_MIPS16(_o)  \
    \
    (((_o) & 0xf0) == STO_MIPS16)


#define ELF_R_SYM(_i)   ((_i) >> 8)
#define ELF_R_TYPE(_i)  ((Elf_Uchar)(_i))


typedef struct
{
    Elf_Uchar  e_ident[EI_NIDENT];  // 识别标志
    
    Elf_Half   e_type;       // 文件类型
    Elf_Half   e_machine;    // 体系结构
    Elf_Word   e_version;    // 文件版本
    Elf_Addr   e_entry;      // 程序入口地址
    Elf_Off    e_phoff;      // 程序头表偏移
    Elf_Off    e_shoff;      // 节头表偏移
    Elf_Word   e_flags;      // 标志
    Elf_Half   e_ehsize;     // 文件头大小
    Elf_Half   e_phentsize;  // 程序头表项大小
    Elf_Half   e_phnum;      // 程序头表项数
    Elf_Half   e_shentsize;  // 节头表项大小
    Elf_Half   e_shnum;      // 节头表项数
    Elf_Half   e_shstrndx;   // 节名表索引
    
} Elf_Ehdr;  // 文件头


typedef struct
{
    Elf_Word   p_type;    // 段类型
    Elf_Off    p_offset;  // 段偏移
    Elf_Addr   p_vaddr;   // 虚拟地址
    Elf_Addr   p_paddr;   // 物理地址
    Elf_Word   p_filesz;  // 文件内容大小
    Elf_Word   p_memsz;   // 内存镜像大小
    Elf_Word   p_flags;   // 段标志
    Elf_Word   p_align;   // 对齐
    
} Elf_Phdr;  // 程序头


typedef struct
{
    Elf_Word   sh_name;       // 节名索引
    Elf_Word   sh_type;       // 节类型
    Elf_Word   sh_flags;      // 节标志
    Elf_Addr   sh_addr;       // 映射地址
    Elf_Off    sh_offset;     // 节偏移
    Elf_Word   sh_size;       // 节大小
    Elf_Word   sh_link;       // 连接索引
    Elf_Word   sh_info;       // 信息
    Elf_Word   sh_addralign;  // 地址对齐
    Elf_Word   sh_entsize;    // 表项大小
    
} Elf_Shdr;  // 节头


typedef struct
{
    Elf_Word   st_name;   // 符号名
    Elf_Addr   st_value;  // 值
    Elf_Word   st_size;   // 大小
    Elf_Uchar  st_info;   // 绑定和类型
    Elf_Uchar  st_other;  // 其它
    Elf_Half   st_shndx;  // 节头索引
    
} Elf_Sym;  // 符号表项


typedef struct
{
    Elf_Addr  r_offset;  // 重定位偏移
    Elf_Word  r_info;    // 重定位信息
    
} Elf_Rel;  // 重定位项


static Elf_Shdr* get_shdrtab(FILE* file, Elf_Ehdr* ehdr)
{
    Elf_Shdr*  shdrtab;
    
    if ((shdrtab = malloc(ehdr->e_shnum * sizeof(Elf_Shdr))) == NULL)
        goto err;

    /* 节头表 */

    fseek(file, ehdr->e_shoff, SEEK_SET);

    fread(shdrtab, sizeof(Elf_Shdr), ehdr->e_shnum, file);

    return shdrtab;
err:
    return NULL;
}


static char* get_shstrtab(FILE* file, Elf_Ehdr* ehdr, Elf_Shdr* shdrtab)
{
    Elf_Shdr*  shdr = &shdrtab[ehdr->e_shstrndx];

    char*  shstrtab;
    
    /* 节名字符串表 */

    if ((shstrtab = malloc(shdr->sh_size)) == NULL)
        goto err;

    fseek(file, shdr->sh_offset, SEEK_SET);

    fread(shstrtab, shdr->sh_size, 1, file);

    return shstrtab;
err:
    return NULL;
}


static char* get_strtab(FILE* file, Elf_Ehdr* ehdr, char* shstrtab, Elf_Shdr* shdrtab)
{
    Elf_Shdr*  shdr = NULL;
    
    char*  strtab;
    int    i;
    
    /* 节头表 */

    for (i = 0; i < ehdr->e_shnum; i++)
    {
        char*  sh_name;

        shdr = &shdrtab[i];
        sh_name = &shstrtab[shdr->sh_name];

        /* 字符串表? */

        if (strcmp(sh_name, ".strtab") == 0)
            break;
    }

    if (i >= ehdr->e_shnum)
        goto err;

    if ((strtab = malloc(shdr->sh_size)) == NULL)
        goto err;

    fseek(file, shdr->sh_offset, SEEK_SET);

    fread(strtab, shdr->sh_size, 1, file);

    return strtab;
err:
    return NULL;
}


static Elf_Sym* get_symtab(FILE* file, Elf_Ehdr* ehdr, char* shstrtab, int* num_syms, Elf_Shdr* shdrtab)
{
    Elf_Shdr*  shdr = NULL;
    
    Elf_Sym*  symtab;
    
    int  i;
    
    /* 节头表 */

    for (i = 0; i < ehdr->e_shnum; i++)
    {
        char*  sh_name;
        
        shdr = &shdrtab[i];
        sh_name = &shstrtab[shdr->sh_name];

        /* 符号表? */

        if (strcmp(sh_name, ".symtab") == 0)
            break;
    }

    if (i >= ehdr->e_shnum)
        goto err;

    if ((symtab = malloc(shdr->sh_size)) == NULL)
        goto err;

    fseek(file, shdr->sh_offset, SEEK_SET);

    fread(symtab, shdr->sh_size, 1, file);

    *num_syms = shdr->sh_size / sizeof(Elf_Sym);

    return symtab;
err:
    return NULL;
}


typedef struct
{
    FILE*     file;
    Elf_Ehdr  ehdr;
    
    char*     shstrtab;
    char*     strtab;
    Elf_Sym*  symtab;
    int       num_syms;

    Elf_Shdr* shdrtab;
    int       shnum;
    
} elf_obj_t;


static FILE* elf_obj_fopen(const char* file_name, Elf_Ehdr* ehdr)
{
    FILE*  file;
    
    if ((file = fopen(file_name, "rb")) == NULL)
        goto err;

    /* 文件头 */
    
    fread(ehdr, sizeof(Elf_Ehdr), 1, file);

    /* 目标文件? */

    if (ehdr->e_type != ET_REL)
        goto err;

    return file;
    
err:
    if (file != NULL)
        fclose(file);
    
    return NULL;
}


static void elf_obj_free(elf_obj_t* obj)
{
    if (obj == NULL)
        return;

    if (obj->file != NULL)
        fclose(obj->file);

    if (obj->symtab != NULL)
        free(obj->symtab);

    if (obj->strtab != NULL)
        free(obj->strtab);

    if (obj->shstrtab != NULL)
        free(obj->shstrtab);

    if (obj->shdrtab != NULL)
        free(obj->shdrtab);
    
    free(obj);
}


static elf_obj_t* elf_obj_load(char* file_name)
{
    elf_obj_t*  obj = malloc(sizeof(elf_obj_t));

    memset(obj, 0, sizeof(elf_obj_t));

    if ((obj->file = elf_obj_fopen(file_name, &obj->ehdr)) == NULL)
        goto err;

    if ((obj->shdrtab = get_shdrtab(obj->file, &obj->ehdr)) == NULL)
        goto err;

    obj->shnum = obj->ehdr.e_shnum;

    if ((obj->shstrtab = get_shstrtab(obj->file, &obj->ehdr, obj->shdrtab)) == NULL)
        goto err;

    if ((obj->strtab = get_strtab(obj->file, &obj->ehdr, obj->shstrtab, obj->shdrtab)) == NULL)
        goto err;

    if ((obj->symtab = get_symtab(obj->file, &obj->ehdr, obj->shstrtab, &obj->num_syms, obj->shdrtab)) == NULL)
        goto err;

    return obj;
    
err:
    elf_obj_free(obj);
    
    return NULL;
}


