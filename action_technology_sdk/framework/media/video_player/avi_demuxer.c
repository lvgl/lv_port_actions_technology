#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avi_demuxer.h"
#include <stream.h>
#include "video_mem.h"

unsigned int Samples_per_sec;

int read4bytes(void *stream)
{
	DWORD val;
	int rdnum;

	rdnum = read_data(stream, &val, 4);
	if (rdnum != 4) {
		val = 0;
	}
	return val;
}


FOURCC check_junk(void *stream, FOURCC tag, FOURCC compare_tag)
{
	int curtag;
	int junklen;
	if (tag != compare_tag)
		return tag;

	junklen = read4bytes(stream);
	read_skip(stream, junklen);
	curtag = read4bytes(stream);
	curtag = check_junk(stream, curtag, compare_tag);
	return curtag;
}

static void dump_tag(FOURCC tag)
{
	printf("[dump_tag] ");
	uint8_t *c = (uint8_t *)&tag;
	for (int i = 0; i < sizeof(FOURCC); i++) {
		printf("%c", c[i]);
	}
	printf("\n");
}

int read_header(void *avict)
{
	//amv len is set to 0 so we can skip zero
	//make it simple just do the amv case
	//riff len avi
	//list len hdrl
	//avih len mainAVIHeader
	//list len strl
	//strh len AVIStreamHeader
	//strf len BITMAPINFORHEADER or WAVEFORMATEX

	//riff len avi or riff 0 amv
	FOURCC tag;
	MainAVIHeader mheader;
	AVIStreamHeader sheader;
	int rdnum;
	//BITMAPINFOHEADER bmpinfo;
	//WAVEFORMATEX waveinfo;
	io_stream_t stream;
	avi_contex_t *ac = NULL;
	//int junklen;
	FOURCC contag;
	int strhlen;
	int strflen;
	int movi_len;
	int movi_off;
	int ret = NORMAL;

	if (avict == NULL) {
		printf("[read_header] avict NULL\n");
		ret = MEMERROR;
		goto ret_handle;
	}

	ac = (avi_contex_t *)avict;

	stream = ac->stream;
 	ac->stream_num = 0;
	read_skip(stream, 8);  //riff len

	tag = read4bytes(stream);
	dump_tag(tag);
	if ((tag != formtypeAVI)&&(tag != formtypeAMV)) {
		printf("[read_header] header tag wrong\n");
		ret = UNKNOWNCON;
		goto ret_handle;
	}

	contag = tag;

	if (contag == formtypeAMV) {
		ac->amvformat = 1;
	}

	read_skip(stream, 12);	//list len hdrl

	tag = read4bytes(stream);
	dump_tag(tag);
	//if (tag != ckidAVIMAINHDR) {	//avih len mainAVIHeader
	//	ret = UNKNOWNCON;
	//	goto ret_handle;
	//}

	read_skip(stream, 4);

	rdnum = read_data(stream, &mheader, sizeof(MainAVIHeader));
	if (rdnum != sizeof(MainAVIHeader)) {
		printf("[read_header] can not find MainAVIHeader\n");
		ret = STREAMEND;
		goto ret_handle;
	}

	if (ac->amvformat) {
		// AMV Format
		int fps = 0;
		int timedur = 0;
		int timeval = mheader.dwReserved[3];

		fps = mheader.dwReserved[0]; //dwRate
		timedur = (timeval&0xff) + ((timeval>>8)&0xff)*60 + ((timeval>>16)&0xff)*3600;

		ac->TotalFrames = timedur * fps;
	 	ac->dwScale = mheader.dwReserved[1];
		ac->dwRate  = mheader.dwReserved[0];
	} else {
		// AVI Format
		ac->TotalFrames = mheader.dwTotalFrames;
	}

	ac->index_flag = mheader.dwFlags;
	ac->bmpheader.biWidth  = mheader.dwWidth;
	ac->bmpheader.biHeight = mheader.dwHeight;

    do {
    	tag = read4bytes(stream);
		dump_tag(tag);
		tag = check_junk(stream, tag, ckidAVIPADDING);

    	if (tag != LISTTAG) {
    		break;
    	}

    	movi_len = read4bytes(stream);
		movi_off = file_tell(stream);
		printf("[read_header] movi_len:0x%x, movi_off:0x%x\n", movi_len, movi_off);
		tag = read4bytes(stream);
		dump_tag(tag);
    	if (tag == listtypeAVIMOVIE) {
    		ac->movitagpos = file_tell(stream) - 4;
			ac->index_offset = movi_len + ac->movitagpos;
			file_seek_set(stream, ac->index_offset);
			tag = read4bytes(stream);
			if (tag != ckidAVINEWINDEX) {
				printf("[read_header] idx1 tag invalid\n");
				ret = UNKNOWNCON;
				goto ret_handle;
			}
			uint32_t index_len = read4bytes(stream);
			ac->framenum = index_len/sizeof(AVIOLDINDEX);
			file_seek_set(stream, ac->movitagpos + 4);
    		break;
    	}

    	tag = read4bytes(stream);
		//strh len AVIStreamHeader
    	if (tag != ckidSTREAMHEADER) {
			printf("[read_header] continue to find stream header\n");
    		file_seek_set(stream, movi_off + movi_len);
			continue;
    	}

		strhlen = read4bytes(stream);
		memset(&sheader, 0, sizeof(AVIStreamHeader));

		if (contag == formtypeAMV) {
			read_skip(stream, strhlen);
		} else {
    		rdnum = read_data(stream, &sheader, sizeof(AVIStreamHeader));
    		if (rdnum != sizeof(AVIStreamHeader)) {
				printf("[read_header] get AVIStreamHeader fail\n");
				ret = STREAMEND;
    			goto ret_handle;
    		}
		}

    	tag = read4bytes(stream);
		//strf len BITMAPINFORHEADER or WAVEFORMATEX
    	if (tag != (ckidSTREAMFORMAT)) {
			printf("[read_header] parse AVIStreamHeader fail\n");
    		break;
    	}

		strflen = read4bytes(stream);
		if (contag == formtypeAMV) {
			if (strflen == 0x14) {
	    		rdnum = read_data(stream, &ac->amvwaveheader, sizeof(AMVAudioStreamFormat));
	    		if (rdnum != sizeof(AMVAudioStreamFormat)) {
					printf("[read_header] parse AMVAudioStreamFormat fail\n");
					ret = STREAMEND;
	    			goto ret_handle;
	    		}
				ac->dwSamples_per_sec = ac->amvwaveheader.dwSamples_per_sec;
				ac->wChannels = ac->amvwaveheader.wChannels;
				Samples_per_sec = ac->dwSamples_per_sec;
			} else {
				read_skip(stream, strflen);
			}
		} else {
	    	if (sheader.fccType == streamtypeVIDEO) {
	 			ac->dwScale = sheader.dwScale;
				ac->dwRate  = sheader.dwRate;

	    		rdnum = read_data(stream, &ac->bmpheader, sizeof(BITMAPINFOHEADER));
	    		if (rdnum != sizeof(BITMAPINFOHEADER)) {
					printf("[read_header] parse BITMAPINFOHEADER fail\n");
					ret = STREAMEND;
	    			goto ret_handle;
	    		}
	    	}

	    	if (sheader.fccType == streamtypeAUDIO) {
	    		rdnum = read_data(stream, &ac->waveheader, sizeof(WAVEFORMATEX));
	    		if (rdnum != sizeof(WAVEFORMATEX)) {
					printf("[read_header] parse WAVEFORMATEX fail\n");
					ret = STREAMEND;
	    			goto ret_handle;
	    		}
				ac->dwSamples_per_sec = ac->waveheader.nSamplesPerSec;
				ac->wChannels = ac->waveheader.nChannels;
				Samples_per_sec = ac->dwSamples_per_sec;
				read_skip(stream, ac->waveheader.cbSize);
	    	}
		}
		ac->stream_num ++;
		file_seek_set(stream, movi_off + movi_len);
    }while(1);

	printf("[read_header] success\n");
ret_handle:
	return ret;
}

int get_es_chunk(void *avict, avi_packet_t *raw_packet)
{
	FOURCC tag = 0;
	int rdnum;
	void *stream;
	avi_contex_t *ac;
	int chunklen = 0;
	int i;
	//int size;

	if ((avict == NULL)||(raw_packet == NULL))
		return MEMERROR;

	ac = (avi_contex_t *)avict;
	stream = ac->stream;
	unsigned char *d = (unsigned char *)&tag;
	for (i = file_tell(stream); i < ac->index_offset; i++) {
		int j;
        for (j = 0; j < sizeof(FOURCC) - 1; j++)
            d[j] = d[j + 1];
		rdnum = read_data(stream, &d[sizeof(FOURCC) - 1], sizeof(unsigned char));
		if (rdnum != sizeof(unsigned char))
			return STREAMEND;

		if (d[0] >= '0' && d[0] <= '9' &&
	        d[1] >= '0' && d[1] <= '9' && ((d[0] - '0') * 10 + (d[1] - '0')) <= ac->stream_num) {
	        //printf("[get_es_chunk] sync success\n");
	    } else {
	        continue;
	    }
		//tag = check_junk(io, tag, ckidAVIPADDING);
		//get chunk type
		switch ((tag >> 16) & DATA_MASK) {
			case cktypeDIBbits:
			case cktypeDIBcompressed:
				raw_packet->es_type = streamtypeVIDEO;
				//printf("f pos: 0x%x\n", file_tell(io) - 4);
				break;
			case cktypeWAVEbytes:
				raw_packet->es_type = streamtypeAUDIO;
				break;
			default:
				raw_packet->es_type = tag;
				break;
		}

		//read chunk
		chunklen = read4bytes(stream);
		break;
	}
	//can not find next frame
	if (chunklen == 0)
		return STREAMEND;
	//printf("[get_es_chunk] tag: 0x%x, file off: 0x%x, chunk len:0x%x\n", tag, file_tell(stream), chunklen);
	raw_packet->data_len = chunklen;
	raw_packet->pts  = ((ac->framecounter * TIMESCALE * 10) / ac->framerate)/10;
	//printf("[get_es_chunk] return normal\n");
	return NORMAL;
}

typedef struct{
	int   frameth;
	DWORD pos;
}frame_info_t;

#define SCAN_BUF_LEN (MAX_PACKET_LEN-3)

int search_es_chunk(void *avict, avi_packet_t *raw_packet)
{
	FOURCC tag;
	//FOURCC Firsttag;
	//int rdnum;
	void *stream;
	avi_contex_t *ac;
	//int chunklen;
	//int d[8];
	//int i,n;
	int size;
	//unsigned char val;
	int frame_num;
	frame_info_t frame_arr[10];
	int offsettostart;
	unsigned char paddbytes[3];
	unsigned char *tmpbuf = 0;
	int end;

	unsigned char *ptr;
	int buf_remain;
	int flag_start;

	if ((avict == NULL)||(raw_packet == NULL))
		return MEMERROR;

	ac = (avi_contex_t *)avict;

	stream = ac->stream;

	flag_start = 0;
	frame_num = 0;
	paddbytes[0] = 0;
	paddbytes[1] = 0;
	paddbytes[2] = 0;

	tmpbuf = raw_packet->data;
GETMOREDATA:
	//ac->movitagpos;

	//offsettostart = file_tell(io);
	offsettostart = file_tell(stream);
	if (offsettostart <= ac->movitagpos)
		return STREAMSTART;

	offsettostart -= ac->movitagpos + 4;

	if (offsettostart > SCAN_BUF_LEN) {
		file_seek_back(stream, SCAN_BUF_LEN);
		read_data(stream, tmpbuf, SCAN_BUF_LEN);
		end = SCAN_BUF_LEN;
		file_seek_back(stream, SCAN_BUF_LEN);
	} else {
		file_seek_back(stream, offsettostart);
		read_data(stream, tmpbuf, offsettostart);
		end = offsettostart;
		flag_start = 1;
		file_seek_back(stream, offsettostart);
	}

	//seek -(5k or less 5k)
	tmpbuf[end + 0] =  paddbytes[0];
	tmpbuf[end + 1] =  paddbytes[1];
	tmpbuf[end + 2] =  paddbytes[2];
	ptr = tmpbuf;
	buf_remain = end + 3;

	while(1) {
		if (buf_remain <= 3) {
			paddbytes[0] = tmpbuf[0];
			paddbytes[1] = tmpbuf[1];
			paddbytes[2] = tmpbuf[2];
			//reget buf;
			if (frame_num)
				goto SACN_END;

			if (flag_start)
				return STREAMSTART;

			goto GETMOREDATA;
		}

		if (*ptr != 0x30) {	//30 is '0'
			ptr++;
			buf_remain--;
			continue;
		}

		if (buf_remain > 3)
			tag = ptr[0] + (ptr[1]<<8) + (ptr[2]<<16) + (ptr[3]<<24);
		else
			continue;

		switch (tag) {
			case 0x63643030:
				frame_arr[frame_num].pos = file_tell(stream) + (ptr - (unsigned char *)tmpbuf);

				if (frame_num >= 10)
					return NORMAL;
				frame_num++;

				if (buf_remain > 7) {
					size = ptr[4] + (ptr[5]<<8) + (ptr[6]<<16) + (ptr[7]<<24);
					if ((buf_remain - 8) >= size) {
						ptr += (8 + size);
						buf_remain -= (8 + size);
					}
					else {
						buf_remain = 0;
					}
				} else {
						ptr += 4;
						buf_remain -=4;
				}

			break;
			default:
				ptr++;
				buf_remain--;
			    break;

		}
	}

SACN_END:
	if (frame_num > 0) {
		int i;
		//get frameth
		for(i = 0; i < frame_num;i++) {
			frame_arr[i].frameth = ac->framecounter - (frame_num - i);
			//save to index
		}
		ac->framecounter = frame_arr[frame_num - 1].frameth;
		file_seek_set(stream, frame_arr[frame_num - 1].pos);
		//printf("b pos: 0x%x\n", frame_arr[frame_num - 1].pos);
		raw_packet->pts  = ((ac->framecounter * TIMESCALE * 10) / ac->framerate )/ 10;
	}

	return NORMAL;

}

