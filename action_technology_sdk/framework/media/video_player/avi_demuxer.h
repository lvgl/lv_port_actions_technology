#ifndef H_AVI_STR
#define H_AVI_STR

#undef BYTE
#undef DWORD
#undef LONG
#undef WORD
#undef FOURCC
#undef NULL

typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned short WORD;
typedef unsigned int FOURCC;         /* a four character code */

#define NULL 0
#define MAX_PACKET_LEN 2048
#define MAX_IDX_CACHE_NUM 100

typedef struct tagRECT {
   WORD left;
   WORD top;
   WORD right;
   WORD bottom;
} RECT;


/**
**	56 bytes
**/
typedef struct
{
    DWORD		dwMicroSecPerFrame;	// frame display rate (or 0L)
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwPaddingGranularity;	// pad to multiples of this
                                                // size; normally 2K.
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwTotalFrames;		// # frames in file
    DWORD		dwInitialFrames;
    DWORD		dwStreams;
    DWORD		dwSuggestedBufferSize;

	// 视频宽度
    DWORD		dwWidth;
	// 视频高度
    DWORD		dwHeight;

    DWORD		dwReserved[4];	//amv 内容为释掉的代码
#if 0
	amv:
	DWORD  dwScale; /* */
	DWORD  dwRate; /* */
	DWORD  dwStart; /* the reference value of starting point for timing */
	DWORD  dwLength; /* the length in the time */
#endif
} MainAVIHeader;

typedef struct {
    FOURCC		fccType;
    FOURCC		fccHandler;
    DWORD		dwFlags;	/* Contains AVITF_* flags */
    WORD		wPriority;
    WORD		wLanguage;
    DWORD		dwInitialFrames;
    DWORD		dwScale;
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwStart;
    DWORD		dwLength; /* In units above... */
    DWORD		dwSuggestedBufferSize;
    DWORD		dwQuality;
    DWORD		dwSampleSize;
    RECT		rcFrame;
} AVIStreamHeader;

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

#pragma pack(2)

typedef struct tWAVEFORMATEX
{
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
    WORD    cbSize;            /* The count in bytes of the size of
                                    extra information (after cbSize) */
} WAVEFORMATEX;

#pragma pack()

/* the format of the audio stream(strf) and its data structure */
typedef struct {
	WORD    wFormat_tag; /* the format of the audio data in this AMV file */
	WORD    wChannels; /* the number of audio channels */
	DWORD   dwSamples_per_sec; /* the number of the samples stored in one second */
	DWORD   dwAvg_bytes_per_sec;	/* samples_per_sec * block_align */
	WORD    wBlock_align;			/* channels * bits_per_sample / 8 */
	WORD    wBits_per_sample; 		/* the number of bits which are used by every sample (8 or 16) */
	WORD    wSize_extra;			/* 0 (Not available temporarily) */
	WORD    reserved;
} AMVAudioStreamFormat;

typedef struct tAVIOLDINDEX {
    FOURCC dwChunkId;         // The four-character code representing the data chunk
    DWORD dwFlags;           // Information about whether this data chunk is a key frame, whether it is a 'rec' list, etc.
    DWORD dwOffset;          // The offset of this data chunk in the file
    DWORD dwSize;            // The size of this data chunk
} AVIOLDINDEX;

typedef enum{
	STREAMSTART = -4,
	STREAMEND,
	UNKNOWNCON,
	MEMERROR,
	NORMAL,
}STATUSINFO;

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

/* Macro to make a TWOCC out of two characters */
#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

/* form types, list types, and chunk types */
#define RIFFTAG             mmioFOURCC('R', 'I', 'F', 'F')
#define LISTTAG             mmioFOURCC('L', 'I', 'S', 'T')
#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define formtypeAMV             mmioFOURCC('A', 'M', 'V', ' ')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define ckidAMVMAINHDR          mmioFOURCC('a', 'm', 'v', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME		mmioFOURCC('s', 't', 'r', 'n')

#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')
#define ckidAMVEND             mmioFOURCC('A', 'M', 'V', '_')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI		mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

/* Basic chunk types */
#define cktypeDIBbits           aviTWOCC('d', 'b')
#define cktypeDIBcompressed     aviTWOCC('d', 'c')
#define cktypePALchange         aviTWOCC('p', 'c')
#define cktypeWAVEbytes         aviTWOCC('w', 'b')

/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')
#define ckidAVIVPRP          mmioFOURCC('v', 'p', 'r', 'p')

#define TIMESCALE 1000

#define DATA_MASK 0x0000ffff

//#define CHUNKPADD(len, amv) ((len)&(!(amv)))
#define CHUNKPADD(len, amv) ((amv)?0:((4 - ((len)&(0x3)))&0x3))

typedef struct
{
	int es_type;
	unsigned int pts;
	unsigned char *data;
	unsigned int data_len;
}avi_packet_t;

typedef struct
{
///////////header info//////////////
	void *stream;
	DWORD	index_offset;
	DWORD	index_flag;
	//DWORD  filetype;
	DWORD  framerate;
	DWORD  framenum;
	DWORD  movitagpos;
	DWORD  TotalFrames;
	//AVIStreamHeader  sheader;
	DWORD  dwScale;
	//帧率，每秒钟的帧数
	DWORD  dwRate;
	unsigned int framecounter;
	DWORD  amvformat;     //len is align 1, must differ from avi
	DWORD  dwSamples_per_sec;
	DWORD  wChannels;
	BITMAPINFOHEADER bmpheader;
	WAVEFORMATEX     waveheader;
	AMVAudioStreamFormat  amvwaveheader;
	unsigned int stream_num;
////////////
}avi_contex_t;


int read_header(void *avict);
int get_es_chunk(void *avict, avi_packet_t *raw_packet);

int read4bytes(void *io);
int search_es_chunk(void *avict, avi_packet_t *raw_packet);


int read_data(void *io, void *buf, unsigned int len);
int read_skip(void *io, int len);
int file_seek_back(void *io, int len);
int file_seek_set(void *io, int len);
int file_tell(void *io);


#endif

