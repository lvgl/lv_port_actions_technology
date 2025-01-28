// define the global val for the adpcm coder
typedef struct {
	short valprev;
	unsigned char index;
	unsigned char reserved;
}adpcm_state;
void adpcm_decoder(char indata[], short outdata[], int len, adpcm_state *state, int avi_flag);

typedef struct {
	int dwSamples_per_sec;
	int wChannels;
} wave_head_t;

typedef struct {
	wave_head_t wave_h;
	adpcm_state state;
} wave_dec_t;
