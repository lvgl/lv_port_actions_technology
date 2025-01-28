/* Generate assembler source containing symbol information
 *
 * Copyright 2002       by Kai Germaschewski
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Usage: nm -n vmlinux | scripts/kallsyms [--all-symbols] > symbols.S
 *
 *      Table compression uses all the unused char codes on the symbols and
 *  maps these to the most used substrings (tokens). For instance, it might
 *  map char code 0xF7 to represent "write_" and then in every symbol where
 *  "write_" appears it can be replaced by 0xF7, saving 5 bytes.
 *      The used codes themselves are also placed in the table so that the
 *  decompresion can work without "special cases".
 *      Applied to kernel symbols, this usually produces a compression ratio
 *  of about 50%.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#define KSYM_NAME_LEN		64

struct sym_entry {
	unsigned long long addr;
	unsigned int len;
	unsigned int start_pos;
	unsigned char *sym;
};

struct text_range {
	const char *stext, *etext;
	unsigned long long start, end;
};

static unsigned long long _text;
static struct text_range text_ranges[] = {
	{ "_image_text_start",     "_image_text_end"     },
	{ "_ramfunc_ram_start",   "_ramfunc_ram_end"     },
};
#define text_range_text			(&text_ranges[0])
#define text_range_text_ramfunc		(&text_ranges[1])

static struct sym_entry *table;
static unsigned int table_size, table_cnt;
static int all_symbols = 0;
static char symbol_prefix_char = '\0';
static unsigned long long kernel_start_addr = 0;
static int no_symbols_name = 0;
static FILE * fbin;

int token_profit[0x10000];

/* the table that holds the result of the compression */
unsigned char best_table[256][2];
unsigned char best_table_len[256];


static void usage(void)
{
	fprintf(stderr, "Usage: kallsyms [--all-symbols] "
			"[--symbol-prefix=<prefix char>] "
			"[--page-offset=<CONFIG_PAGE_OFFSET>] "
			"[--fbin=file "
			"< in.map > out.S\n");
	exit(1);
}

#define SRAM_ADDR 0x1000000
#define check_in_sram(addr)   (addr < (SRAM_ADDR+0x100000))
static void check_text_se(void)
{
	int i;
	struct sym_entry *tbl;
	if(text_range_text->start == 0){
		for (i = 0; i < table_cnt; i++) {
			tbl = &table[i];
			if(check_in_sram(tbl->addr))
				continue;
			if(text_range_text->start == 0){
				text_range_text->start = tbl->addr;
			}
		}
		tbl = &table[i-1];
		text_range_text->end = tbl->addr;
		fprintf(stderr, "text start=0x%llx, end=0x%llx\n", text_range_text->start, text_range_text->end);
	}
		
	if(text_range_text_ramfunc->start == 0){
		for (i = 0; i < table_cnt; i++) {
			tbl = &table[i];
			if(check_in_sram(tbl->addr)){
				if(text_range_text_ramfunc->start == 0)
					text_range_text_ramfunc->start = tbl->addr;
			}else{
				if(text_range_text_ramfunc->start != 0 ){
					break;
				}
			}
		}
		tbl = &table[i-1];
		text_range_text_ramfunc->end = tbl->addr;
		fprintf(stderr, "text ramfunc start=0x%llx, end=0x%llx\n", text_range_text_ramfunc->start, text_range_text_ramfunc->end);
	}
}

/*
 * This ignores the intensely annoying "mapping symbols" found
 * in ARM ELF files: $a, $t and $d.
 */
static inline int is_arm_mapping_symbol(const char *str)
{
	return str[0] == '$' && strchr("atd", str[1])
	       && (str[2] == '\0' || str[2] == '.');
}

static int read_symbol_tr(const char *sym, unsigned long long addr)
{
	size_t i;
	struct text_range *tr;

	for (i = 0; i < ARRAY_SIZE(text_ranges); ++i) {
		tr = &text_ranges[i];

		if (strcmp(sym, tr->stext) == 0) {
			tr->start = addr;
			return 0;
		} else if (strcmp(sym, tr->etext) == 0) {
			tr->end = addr;
			return 0;
		}
	}

	return 1;
}

static int read_symbol(FILE *in, struct sym_entry *s)
{
	char str[500];
	char *sym, stype;
	int rc;

	rc = fscanf(in, "%llx %c %499s\n", &s->addr, &stype, str);
	if (rc != 3) {
		if (rc != EOF && fgets(str, 500, in) == NULL)
			fprintf(stderr, "Read error or end of file.\n");
		return -1;
	}

	sym = str;
	/* skip prefix char */
	if (symbol_prefix_char && str[0] == symbol_prefix_char)
		sym++;

	/* Ignore most absolute/undefined (?) symbols. */
	if (read_symbol_tr(sym, s->addr) == 0)
		/* nothing to do */;
	else if (toupper(stype) == 'A')
	{
#if 0
		/* Keep these useful absolute symbols */
		if (strcmp(sym, "__kernel_syscall_via_break") &&
		    strcmp(sym, "__kernel_syscall_via_epc") &&
		    strcmp(sym, "__kernel_sigtramp") &&
		    strcmp(sym, "__gp"))
			return -1;
#endif
		return -1;
	}
	else if (toupper(stype) == 'U' ||
		is_arm_mapping_symbol(sym))
		return -1;
	/* exclude also MIPS ELF local symbols ($L123 instead of .L123) */
	else if (str[0] == '$')
		return -1;
	/* exclude debugging symbols */
	else if (stype == 'N')
		return -1;
	else if (toupper(stype) == 'D')
		return -1;
	else if (strlen(str) > 64)
		return -1;

	/* include the type field in the symbol name, so that it gets
	 * compressed together */
	s->len = strlen(str) + 1;
	s->sym = malloc(s->len + 1);
	if (!s->sym) {
		fprintf(stderr, "kallsyms failure: "
			"unable to allocate required amount of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy((char *)s->sym + 1, str);
	s->sym[0] = stype;

	return 0;
}

static int symbol_valid_tr(struct sym_entry *s)
{
	size_t i;
	struct text_range *tr;

	for (i = 0; i < ARRAY_SIZE(text_ranges); ++i) {
		tr = &text_ranges[i];

		if (s->addr >= tr->start && s->addr <= tr->end)
			return 1;
	}

	return 0;
}

static int symbol_valid(struct sym_entry *s)
{
	/* Symbols which vary between passes.  Passes 1 and 2 must have
	 * identical symbol lists.  The kallsyms_* symbols below are only added
	 * after pass 1, they would be included in pass 2 when --all-symbols is
	 * specified so exclude them to get a stable symbol list.
	 */
	static char *special_symbols[] = {
		"kallsyms_addresses",
		"kallsyms_num_syms",
		"kallsyms_names",
		"kallsyms_markers",
		"kallsyms_token_table",
		"kallsyms_token_index",

	/* Exclude linker generated symbols which vary between passes */
		"_SDA_BASE_",		/* ppc */
		"_SDA2_BASE_",		/* ppc */
		NULL };
	int i;
	int offset = 1;

	if (s->addr < kernel_start_addr)
		return 0;

	/* skip prefix char */
	if (symbol_prefix_char && *(s->sym + 1) == symbol_prefix_char)
		offset++;

	/* if --all-symbols is not specified, then symbols outside the text
	 * and inittext sections are discarded */
	if (!all_symbols) {
		if (symbol_valid_tr(s) == 0)
			return 0;
		/* Corner case.  Discard any symbols with the same value as
		 * _etext _einittext; they can move between pass 1 and 2 when
		 * the kallsyms data are added.  If these symbols move then
		 * they may get dropped in pass 2, which breaks the kallsyms
		 * rules.
		 */
		if ((s->addr == text_range_text->end &&
				strcmp((char *)s->sym + offset, text_range_text->etext)) ||
		    (s->addr == text_range_text_ramfunc->end &&
				strcmp((char *)s->sym + offset, text_range_text_ramfunc->etext)))
			return 0;
	}

	/* Exclude symbols which vary between passes. */
	if (strstr((char *)s->sym + offset, "_compiled."))
		return 0;

	for (i = 0; special_symbols[i]; i++)
		if( strcmp((char *)s->sym + offset, special_symbols[i]) == 0 )
			return 0;

	return 1;
}

static void read_map(FILE *in)
{
	while (!feof(in)) {
		if (table_cnt >= table_size) {
			table_size += 10000;
			table = realloc(table, sizeof(*table) * table_size);
			if (!table) {
				fprintf(stderr, "out of memory\n");
				exit (1);
			}
		}
		if (read_symbol(in, &table[table_cnt]) == 0) {
			table[table_cnt].start_pos = table_cnt;
			table_cnt++;
		}
	}

	_text = text_range_text->start;
}

static void output_label(char *label)
{
	if (symbol_prefix_char)
		printf(".globl %c%s\n", symbol_prefix_char, label);
	else
		printf(".globl %s\n", label);
	printf("\tALGN\n");
	if (symbol_prefix_char)
		printf("%c%s:\n", symbol_prefix_char, label);
	else
		printf("%s:\n", label);
}


/* uncompress a compressed symbol. When this function is called, the best table
 * might still be compressed itself, so the function needs to be recursive */
static int expand_symbol(unsigned char *data, int len, char *result)
{
	int c, rlen, total=0;

	while (len) {
		c = *data;
		/* if the table holds a single char that is the same as the one
		 * we are looking for, then end the search */
		if (best_table[c][0]==c && best_table_len[c]==1) {
			*result++ = c;
			total++;
		} else {
			/* if not, recurse and expand */
			rlen = expand_symbol(best_table[c], best_table_len[c], result);
			total += rlen;
			result += rlen;
		}
		data++;
		len--;
	}
	*result=0;

	return total;
}


//TLV
/*
0x0- 0x03  magic
0x04-0x07  len
0x08-0x0b   checksum
0x0c-0x10  resrve
0x10---  tlv
*/
#define TLV_MAGIC				0x59355935
#define TYPE_KYMS_ADDR          0x01             //kallsyms_addresses
#define TYPE_KYMS_NUM           0x02             //kallsyms_num_syms
#define TYPE_KYMS_NAME          0x03             //kallsyms_names
#define TYPE_KYMS_MARKERS       0x04             //kallsyms_markers
#define TYPE_KYMS_TOKEN_TABEL   0x05             //kallsyms_token_table
#define TYPE_KYMS_TOKEN_INDEX   0x06             //kallsyms_token_index




int fbin_write(const void *buf, int len)
{
	if(fbin != NULL){
		if(fwrite(buf,len, 1, fbin) <=0)
			fprintf(stderr,"fbin write err\n");
	}
	return 0;
}
int fbin_addr(unsigned int addr)
{
	return fbin_write(&addr,4);
}
int fbin_byte(unsigned char ch)
{
	return fbin_write(&ch,1);
}
int fbin_short(unsigned short ch)
{
	return fbin_write(&ch,2);
}

int fbin_seek(int offset)
{
	if(fbin != NULL)
		return fseek(fbin, offset, SEEK_SET);
	return  0;
}
int fbin_tell(void)
{
	if(fbin != NULL)
		return ftell(fbin);
	return 0;
}
int fbin_type_len(unsigned int type, unsigned int len, int offset)
{
	unsigned int buf[2],tlen;
	char cbuf[4];
	if(fbin == NULL)
		return 0;
	if(len&0x3){//algin 4
		tlen = 4- (len&0x3);
		memset(cbuf,0,4);
		fbin_write(cbuf, tlen);
		fprintf(stderr,"algin =0x%x, panding=0x%x\n", len, tlen);
		len += tlen;
		
	}
	buf[0] = type;
	buf[1] = len;
	fbin_seek(offset);
	fbin_write(buf, 8);
	fprintf(stderr,"offset=0x%x,type=%d,len=0x%x\n", offset, type, len);
	fseek(fbin, 0, SEEK_END);
	return 0;
}

void fbin_init(void)
{
	if(fbin == NULL)
		return ;
	fbin_addr(TLV_MAGIC);
	fbin_addr(0);
	fbin_addr(0);
	fbin_addr(0);
}

void fbin_end(void)
{
	unsigned int *pbuf;
	unsigned int len ,ck, ret, i;
	if(fbin == NULL)
		return ;

	fseek(fbin, 0, SEEK_END);
	len = ftell(fbin);
	pbuf = (unsigned int *)malloc(len+4);
	if(pbuf == NULL){
		fprintf(stderr,"malloc %d fail\n", len);
		return;
	}
	fseek(fbin, 0, SEEK_SET);
	ret = fread(pbuf,len, 1, fbin);
	if((ret != 1) || (len < 16)) {
		fprintf(stderr,"read fail, len=0x%x, ret=%d\n", len, ret);
		free(pbuf);
		return;
	}
	ck = 0;
	for(i = 4; i < len/4; i++)
		ck += pbuf[i];
	fseek(fbin, 4, SEEK_SET);
	fbin_addr(len-16);
	fbin_addr(ck);
	fprintf(stderr,"ok, len=0x%x, ck=0x%x\n", len, ck);
	free(pbuf);
	fclose(fbin);
}

static FILE * ftbl;

static void  ftbl_init(void)
{
	int i, fline=0;
	struct sym_entry *tbl;
	ftbl = fopen("ktbl.map","w");
	if(ftbl != NULL)
		fprintf(stderr, "create table file ok\n");
	else
		fprintf(stderr, "create table file error\n");
	for (i = 0; i < table_cnt; i++) {
		tbl = &table[i];
		fprintf(ftbl, "0x%08x 0x%08llx %s\n", fline++, tbl->addr, (char*)(tbl->sym+1));
	}

	fclose(ftbl);
}

static void write_src(void)
{
	unsigned int i, k, off,foffset, f_start;
	unsigned int best_idx[256];
	unsigned int *markers;
	char buf[KSYM_NAME_LEN];

//	printf("#include <asm/types.h>\n");
	printf("#if BITS_PER_LONG == 64\n");
	printf("#define PTR .quad\n");
	printf("#define ALGN .align 8\n");
	printf("#else\n");
	printf("#define PTR .long\n");
	printf("#define ALGN .align 4\n");
	printf("#endif\n");

	printf("\t.section .rodata, \"a\"\n");

	/* Provide proper symbols relocatability by their '_text'
	 * relativeness.  The symbol names cannot be used to construct
	 * normal symbol references as the list of symbols contains
	 * symbols that are declared static and are private to their
	 * .o files.  This prevents .tmp_kallsyms.o or any other
	 * object from referencing them.
	 */
	fbin_init();
	f_start = 0x10;
	fbin_type_len(TYPE_KYMS_ADDR, 0, f_start);
	output_label("kallsyms_addresses");
	for (i = 0; i < table_cnt; i++) {
		if (toupper(table[i].sym[0]) != 'A') {
			if (_text <= table[i].addr)
				printf("\tPTR\t_image_text_start + %#llx\n",
					table[i].addr - _text);
			else
				printf("\tPTR\t_image_text_start - %#llx\n",
					_text - table[i].addr);
		} else {
			printf("\tPTR\t%#llx\n", table[i].addr);
		}
		fbin_addr(table[i].addr);
	}

	printf("\n");
	foffset = fbin_tell();
	fbin_type_len(TYPE_KYMS_ADDR, foffset-f_start-8, f_start);
	f_start = fbin_tell();

	fbin_type_len(TYPE_KYMS_NUM, 0, f_start);
	output_label("kallsyms_num_syms");
	printf("\tPTR\t%d\n", table_cnt);
	printf("\n");
	fbin_addr(table_cnt);
	foffset = fbin_tell();
	fbin_type_len(TYPE_KYMS_NUM, foffset-f_start-8, f_start);
	f_start = fbin_tell();

	/* table of offset markers, that give the offset in the compressed stream
	 * every 256 symbols */
	markers = malloc(sizeof(unsigned int) * ((table_cnt + 255) / 256));
	if (!markers) {
		fprintf(stderr, "kallsyms failure: "
			"unable to allocate required memory\n");
		exit(EXIT_FAILURE);
	}

	if (!no_symbols_name) {
		fbin_type_len(TYPE_KYMS_NAME, 0, f_start);
		output_label("kallsyms_names");
		off = 0;
		for (i = 0; i < table_cnt; i++) {
			if ((i & 0xFF) == 0)
				markers[i >> 8] = off;

			printf("\t.byte 0x%02x", table[i].len);
			fbin_byte(table[i].len);
			for (k = 0; k < table[i].len; k++)
				printf(", 0x%02x", table[i].sym[k]);
			printf("\n");
			fbin_write(table[i].sym, table[i].len);
			off += table[i].len + 1;
		}
		printf("\n");
		foffset = fbin_tell();
		fbin_type_len(TYPE_KYMS_NAME, foffset-f_start-8, f_start);
		f_start = fbin_tell();
	}

	fbin_type_len(TYPE_KYMS_MARKERS, 0, f_start);
	output_label("kallsyms_markers");
	for (i = 0; i < ((table_cnt + 255) >> 8); i++){
		printf("\tPTR\t%d\n", markers[i]);
		fbin_addr(markers[i]);
	}
	printf("\n");

	free(markers);
	foffset = fbin_tell();
	fbin_type_len(TYPE_KYMS_MARKERS, foffset-f_start-8, f_start);
	f_start = fbin_tell();

	fbin_type_len(TYPE_KYMS_TOKEN_TABEL, 0, f_start);
	output_label("kallsyms_token_table");
	off = 0;
	for (i = 0; i < 256; i++) {
		best_idx[i] = off;
		expand_symbol(best_table[i], best_table_len[i], buf);
		printf("\t.asciz\t\"%s\"\n", buf);
		fbin_write(buf, strlen(buf) + 1);
		off += strlen(buf) + 1;
	}
	printf("\n");
	foffset = fbin_tell();
	fbin_type_len(TYPE_KYMS_TOKEN_TABEL, foffset-f_start-8, f_start);
	f_start = fbin_tell();

	fbin_type_len(TYPE_KYMS_TOKEN_INDEX, 0, f_start);
	output_label("kallsyms_token_index");
	for (i = 0; i < 256; i++){
		printf("\t.short\t%d\n", best_idx[i]);
		fbin_short(best_idx[i]);
	}
	printf("\n");
	foffset = fbin_tell();
	fbin_type_len(TYPE_KYMS_TOKEN_INDEX, foffset-f_start-8, f_start);
	f_start = fbin_tell();
	fbin_end();
}


/* table lookup compression functions */

/* count all the possible tokens in a symbol */
static void learn_symbol(unsigned char *symbol, int len)
{
	int i;

	for (i = 0; i < len - 1; i++)
		token_profit[ symbol[i] + (symbol[i + 1] << 8) ]++;
}

/* decrease the count for all the possible tokens in a symbol */
static void forget_symbol(unsigned char *symbol, int len)
{
	int i;

	for (i = 0; i < len - 1; i++)
		token_profit[ symbol[i] + (symbol[i + 1] << 8) ]--;
}

/* remove all the invalid symbols from the table and do the initial token count */
static void build_initial_tok_table(void)
{
	unsigned int i, pos;

	pos = 0;
	for (i = 0; i < table_cnt; i++) {
		if ( symbol_valid(&table[i]) ) {
			if (pos != i)
				table[pos] = table[i];
			learn_symbol(table[pos].sym, table[pos].len);
			pos++;
		}
	}
	table_cnt = pos;
}

static void *find_token(unsigned char *str, int len, unsigned char *token)
{
	int i;

	for (i = 0; i < len - 1; i++) {
		if (str[i] == token[0] && str[i+1] == token[1])
			return &str[i];
	}
	return NULL;
}

/* replace a given token in all the valid symbols. Use the sampled symbols
 * to update the counts */
static void compress_symbols(unsigned char *str, int idx)
{
	unsigned int i, len, size;
	unsigned char *p1, *p2;

	for (i = 0; i < table_cnt; i++) {

		len = table[i].len;
		p1 = table[i].sym;

		/* find the token on the symbol */
		p2 = find_token(p1, len, str);
		if (!p2) continue;

		/* decrease the counts for this symbol's tokens */
		forget_symbol(table[i].sym, len);

		size = len;

		do {
			*p2 = idx;
			p2++;
			size -= (p2 - p1);
			memmove(p2, p2 + 1, size);
			p1 = p2;
			len--;

			if (size < 2) break;

			/* find the token on the symbol */
			p2 = find_token(p1, size, str);

		} while (p2);

		table[i].len = len;

		/* increase the counts for this symbol's new tokens */
		learn_symbol(table[i].sym, len);
	}
}

/* search the token with the maximum profit */
static int find_best_token(void)
{
	int i, best, bestprofit;

	bestprofit=-10000;
	best = 0;

	for (i = 0; i < 0x10000; i++) {
		if (token_profit[i] > bestprofit) {
			best = i;
			bestprofit = token_profit[i];
		}
	}
	return best;
}

/* this is the core of the algorithm: calculate the "best" table */
static void optimize_result(void)
{
	int i, best;

	/* using the '\0' symbol last allows compress_symbols to use standard
	 * fast string functions */
	for (i = 255; i >= 0; i--) {

		/* if this table slot is empty (it is not used by an actual
		 * original char code */
		if (!best_table_len[i]) {

			/* find the token with the breates profit value */
			best = find_best_token();
			if (token_profit[best] == 0)
				break;

			/* place it in the "best" table */
			best_table_len[i] = 2;
			best_table[i][0] = best & 0xFF;
			best_table[i][1] = (best >> 8) & 0xFF;

			/* replace this token in all the valid symbols */
			compress_symbols(best_table[i], i);
		}
	}
}

/* start by placing the symbols that are actually used on the table */
static void insert_real_symbols_in_table(void)
{
	unsigned int i, j, c;

	memset(best_table, 0, sizeof(best_table));
	memset(best_table_len, 0, sizeof(best_table_len));

	for (i = 0; i < table_cnt; i++) {
		for (j = 0; j < table[i].len; j++) {
			c = table[i].sym[j];
			best_table[c][0]=c;
			best_table_len[c]=1;
		}
	}
}

static void optimize_token_table(void)
{
	build_initial_tok_table();

	insert_real_symbols_in_table();

	/* When valid symbol is not registered, exit to error */
	if (!table_cnt) {
		fprintf(stderr, "No valid symbol.\n");
		exit(1);
	}

	optimize_result();
}

/* guess for "linker script provide" symbol */
static int may_be_linker_script_provide_symbol(const struct sym_entry *se)
{
	const char *symbol = (char *)se->sym + 1;
	int len = se->len - 1;

	if (len < 8)
		return 0;

	if (symbol[0] != '_' || symbol[1] != '_')
		return 0;

	/* __start_XXXXX */
	if (!memcmp(symbol + 2, "start_", 6))
		return 1;

	/* __stop_XXXXX */
	if (!memcmp(symbol + 2, "stop_", 5))
		return 1;

	/* __end_XXXXX */
	if (!memcmp(symbol + 2, "end_", 4))
		return 1;

	/* __XXXXX_start */
	if (!memcmp(symbol + len - 6, "_start", 6))
		return 1;

	/* __XXXXX_end */
	if (!memcmp(symbol + len - 4, "_end", 4))
		return 1;

	return 0;
}

static int prefix_underscores_count(const char *str)
{
	const char *tail = str;

	while (*tail == '_')
		tail++;

	return tail - str;
}

static int compare_symbols(const void *a, const void *b)
{
	const struct sym_entry *sa;
	const struct sym_entry *sb;
	int wa, wb;

	sa = a;
	sb = b;

	/* sort by address first */
	if (sa->addr > sb->addr)
		return 1;
	if (sa->addr < sb->addr)
		return -1;

	/* sort by "weakness" type */
	wa = (sa->sym[0] == 'w') || (sa->sym[0] == 'W');
	wb = (sb->sym[0] == 'w') || (sb->sym[0] == 'W');
	if (wa != wb)
		return wa - wb;

	/* sort by "linker script provide" type */
	wa = may_be_linker_script_provide_symbol(sa);
	wb = may_be_linker_script_provide_symbol(sb);
	if (wa != wb)
		return wa - wb;

	/* sort by the number of prefix underscores */
	wa = prefix_underscores_count((const char *)sa->sym + 1);
	wb = prefix_underscores_count((const char *)sb->sym + 1);
	if (wa != wb)
		return wa - wb;

	/* sort by initial order, so that other symbols are left undisturbed */
	return sa->start_pos - sb->start_pos;
}

static void sort_symbols(void)
{
	qsort(table, table_cnt, sizeof(struct sym_entry), compare_symbols);
	check_text_se();
}

int main(int argc, char **argv)
{
	fbin = NULL;
	if (argc >= 2) {
		int i;
		for (i = 1; i < argc; i++) {
			if(strcmp(argv[i], "--all-symbols") == 0)
				all_symbols = 1;
			else if (strncmp(argv[i], "--symbol-prefix=", 16) == 0) {
				char *p = &argv[i][16];
				/* skip quote */
				if ((*p == '"' && *(p+2) == '"') || (*p == '\'' && *(p+2) == '\''))
					p++;
				symbol_prefix_char = *p;
			} else if (strncmp(argv[i], "--page-offset=", 14) == 0) {
				const char *p = &argv[i][14];
				kernel_start_addr = strtoull(p, NULL, 16);
			} else if(strcmp(argv[i], "--no_symbols_name") == 0) {
				no_symbols_name = 1;
			}else if(strncmp(argv[i], "--fbin=", 7) == 0) {
				fbin = fopen(&argv[i][7],"wb+");
				if(fbin != NULL)
					fprintf(stderr, "create file =%s ok\n", &argv[i][7]);
				else
					fprintf(stderr, "create file =%s error\n", &argv[i][7]);
			}
			else
				usage();
		}
	} else if (argc != 1)
		usage();

	read_map(stdin);
	sort_symbols();
	ftbl_init();
	optimize_token_table();
	write_src();

	return 0;
}
