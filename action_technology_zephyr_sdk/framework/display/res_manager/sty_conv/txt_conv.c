#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "res_struct.h"

typedef struct
{
    uint8_t  magic[4];        //'R', 'E', 'S', 0x19
    uint16_t counts;
    uint16_t w_format;
    uint8_t brgb;
    uint8_t version;
	uint16_t id_start;
	uint8_t reserved;
	uint8_t tile_compress;
	uint8_t compressed_type;
    uint8_t ch_extend;
}res_head_t;

typedef struct
{
    uint32_t   offset;
    uint16_t   length;
    uint8_t    type;
    uint8_t    name[8];
    uint8_t    len_ext;
}res_entry_t;

int _get_text_array_from_id(FILE* fp, int id, uint8_t* text, int text_len)
{
    res_entry_t res_entry;
    int ret;
    uint8_t* buffer;
    int i;
    uint8_t inbuf[16] = {0};

    memset(&res_entry, 0, sizeof(res_entry_t));
	fseek(fp, id * sizeof(res_entry_t), SEEK_SET);
	ret = fread(&res_entry, 1, sizeof(res_entry_t), fp);
    if(ret < sizeof(res_entry_t))
    {
    	printf("error: cannot find string resource for id %d\n", id);
    	return -1;
    }

	fseek(fp, res_entry.offset, SEEK_SET);

    memset(text, 0, text_len);
    buffer = (uint8_t*)malloc(res_entry.length+1);
    memset(buffer, 0, res_entry.length+1);
    ret = fread(buffer, 1, res_entry.length, fp);
    if(ret < res_entry.length)
    {
        printf("error: unfit string length %d vs %d\n", ret, res_entry.length);
    }

    for(i=0;i<res_entry.length;i++)
    {
        memset(inbuf, 0, 16);
        if(i < res_entry.length - 1)
        {
            sprintf(inbuf, "0x%x, ", buffer[i]);
        }
        else
        {
            sprintf(inbuf, "0x%x", buffer[i]);
        }
        strcat(text, inbuf);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int ret; 
    FILE* infp;
    FILE* outfp;
    uint8_t cname[256] = {0};
    uint8_t vname[256] = {0};
    uint8_t out_path[256] = {0};
    uint8_t* off;
    res_head_t res_head;
    uint8_t inbuf[2048] = {0};
    uint8_t arraybuf[2048] = {0};
    int item_cnt;
    FILE* includefp;

    if(argc < 3)
    {
        printf("illegal params\n");
        return -1;
    }

    includefp = fopen(argv[2], "a+");
    if(includefp == NULL)
    {
        printf("open include %s failed\n", argv[2]);
        return -1;    
    }


    printf("%s %d: %s\n", __FILE__, __LINE__, argv[1]);
    strcpy(cname, argv[1]);
    infp = fopen(argv[1], "rb");
    if(infp == NULL)
    {
        printf("open input %s failed\n", argv[1]);
        fclose(includefp);
        return -1;
    }

    printf("%s %d\n", __FILE__, __LINE__);
	fseek(infp, 0, SEEK_SET);
	memset(&res_head, 0, sizeof(res_head));
	ret = fread(&res_head, 1, sizeof(res_head), infp);
    if (ret < sizeof(res_head) || res_head.version != 2)
    {
        //support utf8 only for now
        printf("error: unsupported encoding %d\n", res_head.version);
        fclose(infp);
        fclose(includefp);
        return -1;
    }    
    item_cnt = res_head.counts;

    printf("%s %d\n", __FILE__, __LINE__);
    //generate output path & variable name
    strcpy(out_path, argv[2]);
    off = strrchr(out_path, '\\');
    if(off == NULL)
    {
        off = strrchr(out_path, '/');
    }
    if(off != NULL)
    {
        *off = 0;
    }
    else
    {
        memset(out_path, 0, 256);
    }

    off = strrchr(cname, '\\');
    if(off == NULL)
    {
        off = strrchr(cname, '/');
    }
    printf("off %p\n", off);
    if(off == NULL)
    {
        off = argv[1];
    }
    else
    {
        off++;
    }
    strcat(out_path, "/");
    strcat(out_path, off);

    strcpy(vname, off);
    off = strrchr(vname, '.');
    *off = '_';
    printf("vname %s\n", vname);

    printf("%s %d\n", __FILE__, __LINE__);

    off = strrchr(out_path, '.');
    *off = '_';
    strcat(out_path, ".c");        
    outfp = fopen(out_path, "wb");
    if(outfp == NULL)
    {
        printf("open output %s failed\n", out_path);
        fclose(infp);
        fclose(includefp);
        return -1;
    }

    printf("%s %d : %s\n", __FILE__, __LINE__, out_path);
    sprintf(inbuf, "#include <res_manager_api.h> \n\n");
    fwrite(inbuf, 1, strlen(inbuf), outfp);

    int i;
    for(i=1;i<=item_cnt;i++)
    {
        uint8_t text[2048] = {0};

        _get_text_array_from_id(infp, i, text, 2048);

        memset(inbuf, 0, 2048);
        sprintf(inbuf, "static const uint8_t str_%d[] = {%s}; \n\n", i, text);
        fwrite(inbuf, 1, strlen(inbuf), outfp);

        if(i==1)
        {
            sprintf(arraybuf, "str_%d", i);
        }
        else
        {
            sprintf(arraybuf, "%s, str_%d", arraybuf, i);
        }

    }

    memset(inbuf, 0, 2048);
    sprintf(inbuf, "const uint8_t* res_string_%s[%d] = {%s}; \n", vname, item_cnt, arraybuf);
    fwrite(inbuf, 1, strlen(inbuf), outfp);       

    memset(inbuf, 0, 2048);
    sprintf(inbuf, "uint32_t res_string_%s_cnt = %d; \n", vname, item_cnt);
    fwrite(inbuf, 1, strlen(inbuf), outfp);       

    fseek(includefp, 0, SEEK_END);
    memset(inbuf, 0, 2048);
    sprintf(inbuf, "\nextern const uint8_t* res_string_%s[];\nextern uint32_t res_string_%s_cnt;\n", vname, vname);
    fwrite(inbuf, 1, strlen(inbuf), includefp);


    fclose(infp);
    fclose(outfp);
    fclose(includefp);
    return 0;
}