#include <stdint.h>

typedef enum 
{
	RESOURCE_TYPE_GROUP=3,
	RESOURCE_TYPE_PICTURE=4,
	RESOURCE_TYPE_TEXT=2,
	RESOURCE_TYPE_PICREGION=1, //fixme
} resource_type_e;

//res type
typedef enum{
    RES_TYPE_INVALID                 = 0,
    RES_TYPE_PIC                     = 1,  // 16 bit picture (RGB_565, R[15:11], G[10:5], B[4:0])
    RES_TYPE_STRING                  = 2,  // string
    RES_TYPE_MSTRING,                      // multi-Language string
    RES_TYPE_LOGO                    = 4,  // BIN format picture
    RES_TYPE_PNG,                          // 24 bit picture with alpha (ARGB_8565, A[23:16], R[15:11], G[10:5], B[4:0])
    RES_TYPE_PIC_4,
    RES_TYPE_PIC_COMPRESSED          = 7,  // compressed 16 bit picture (compressed RGB_565)
    RES_TYPE_PNG_COMPRESSED,               // compressed 24 bit compressed picture with alpha (compressed ARGB_8565)
    RES_TYPE_LOGO_COMPRESSED,              // compressed BIN format picture

    RES_TYPE_PIC_RGB888              = 10, // 24 bit picture without alpha (RGB_888, R[23:16], G[16:8], B[7:0])
    RES_TYPE_PIC_ARGB8888,                 // 32 bit picture with alpha (ARGB_8888, , A[31:24], R[23:16], G[16:8], B[7:0])
    RES_TYPE_PIC_RGB888_COMPRESSED,        // compressed 24 bit picture without alpha (RGB_888)
    RES_TYPE_PIC_ARGB8888_COMPRESSED = 13, // compressed 32 bit picture with alpha (ARGB_8888)

    RES_TYPE_RGB565_LZ4,       				// LZ4 compressed of RGB565
    RES_TYPE_PIC_ARGB8888_LZ4,  			// 32 bit picture with alpha (ARGB_8888, , A[31:24], R[23:16], G[16:8], B[7:0]) with LZ4 compressed

    RES_TYPE_A8 = 30,                 			// Raw format of A8
    RES_TYPE_A8_LZ4,             			// LZ4 compressed format of A8

    RES_TYPE_ARGB6666,           			// Raw format of ARGB6666
    RES_TYPE_ARGB6666_LZ4,       			// LZ4 compressed of ARGB6666

	RES_TYPE_ARGB565,       			// Raw compressed of ARGB565
    RES_TYPE_ARGB565_LZ4,       			// LZ4 compressed of ARGB565

	RES_TYPE_PIC_ARGB1555,			 		// Raw format of ARGB1555
    RES_TYPE_ARGB1555_LZ4,       			// LZ4 compressed of ARGB1555

    RES_TYPE_JPEG,              			// jpeg format

    RES_TYPE_TILE_COMPRESSED,  				// tile compressed format

    NUM_RES_TYPE,
}res_type_e;

typedef enum
{
	RESOURCE_BITMAP_FORMAT_RGB565,
	RESOURCE_BITMAP_FORMAT_ARGB8565,
	RESOURCE_BITMAP_FORMAT_RGB888,
	RESOURCE_BITMAP_FORMAT_ARGB8888,
	RESOURCE_BITMAP_FORMAT_A8,
	RESOURCE_BITMAP_FORMAT_ARGB6666,
	RESOURCE_BITMAP_FORMAT_RAW,
	RESOURCE_BITMAP_FORMAT_JPEG,
	RESOURCE_BITMAP_FORMAT_ARGB1555,
} resource_bitmap_format_e;

/*!
*\brief
    data structure sty header
*/
typedef struct style_s
{
	/*! total scene number*/
	uint32_t sum; 

	/*! offset to scene data, only used in conv tool*/
	int32_t* scenes;
} resource_info_t;

/*!
*\brief
    data structure of scene resource
*/
typedef struct
{		  
	/** x coordinate */
	int16_t  x;
	/** y coordinate */
	int16_t  y;
	/** scene width	*/
	int16_t  width;
	/** scene height */
	int16_t  height;
	/** scene background color */
	uint32_t background;	 
	/** scene transparencty value*/
	uint32_t  transparence;
	/** total resource number of scene */
	uint32_t resource_sum;
	/** offset to first child resource */
	uint32_t child_offset;
	/** scene id*/
	uint32_t scene_id;
	/** scene layout mode, horizontal or vertical*/
	uint16_t direction;	
	/** visible flag */
	uint16_t  visible;
	/*! opaque flag */
	uint16_t   opaque;	
	uint16_t   reserve;
} resource_scene_t;



/*!
*\brief
    data structure of group resource
*/
typedef struct
{
	/** resource base type*/
	uint32_t type;
	/** hashed id of the group*/
	uint32_t sty_id;
	/** offset to next resource*/
	uint32_t offset;
	/** x coordinate to parent resource*/
	int16_t x;
	/** y coordinate to parent resource */
	int16_t y;   
	/** group width */
	uint16_t width;
	/** group height*/
	uint16_t height;
	/** group background color*/
	uint32_t backgroud;
	/** opaque flag*/
	uint32_t opaque;
	/** total resource number of the group*/
	uint32_t resource_sum;
	/** offset to first child resource */
	uint32_t child_offset;	
} sty_group_t;

/*!
*\brief
    data structure of picture resource
*/
typedef struct
{
	/** resource base type */
	uint32_t type;	
	/** hased id of the resource */
	uint32_t sty_id;	
	/** offset to next resource */
	uint32_t offset;
	/** res id in pic res file*/
	uint32_t id;
	/** x coordinate to parent resource */
	int16_t x;
	/** y coordinate to parent resource */
	int16_t y;
	/** pic width */
	uint16_t width;
	/** pic height */
	uint16_t height;
	/** bytes per pixel*/
	uint16_t bytes_per_pixel;
	/** bitmap format*/
	uint16_t format;
	/** bitmap data offset in res file*/
	uint32_t bmp_pos;
	/** compressed data size, 0 if uncompressed*/
	uint32_t compress_size;
	/** magic data for regular info comparing*/
	uint32_t magic;
} sty_picture_t;

/*!
*\brief
    data structure of string resource
*/
typedef struct
{
	/** resource type*/
	uint32_t type;
	/** hashed sty id*/
	uint32_t sty_id;	
	/** offset to text content*/
	uint32_t offset;	
	/** resource id*/
	uint32_t id;
	/** x coord to parent  */
	int16_t x;
	/** y coord to parent  */
	int16_t y;
	/** text area width  */
	uint16_t width;
	/** text area height  */
	uint16_t height;
	/** font size  */
	uint16_t font_size;
	/** text algin mode */
	uint16_t align;
	/** text color  */
	uint32_t color;
	/** string background color*/
	uint32_t bgcolor;
} sty_string_t;

/*!
*\brief
    data structure of pic region resource
*/
typedef struct
{
	uint32_t type;
	/** hashed id of the resource */
	uint32_t sty_id;	
	uint32_t offset;	
 	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t format;	
	uint16_t bytes_per_pixel;
	uint32_t frames;
	uint32_t id_offset;
	uint32_t magic;
}sty_picregion_t;

/*
 * bitmap struct for style src code
*/
typedef struct
{
	/* data */
	uint32_t type;	
	uint32_t sty_id;	
	uint32_t id;
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t bytes_per_pixel;
	uint16_t format;
	uint32_t bmp_pos;
	uint32_t compress_size;
	uint32_t magic;	
	uint8_t* buffer;
	void* res_info;
}style_bitmap_t;

/*
 * string struct for style src code
*/
typedef struct
{
	/** resource type*/
	uint32_t type;
	/** hashed sty id*/
	uint32_t sty_id;		
	/** resource id*/
	uint32_t id;
	/** x coord to parent  */
	int16_t x;
	/** y coord to parent  */
	int16_t y;
	/** text area width  */
	uint16_t width;
	/** text area height  */
	uint16_t height;
	/** font size  */
	uint16_t font_size;
	/** text algin mode */
	uint16_t align;
	/** text color  */
	uint32_t color;
	/** string background color*/
	uint32_t bgcolor;	
	uint8_t* buffer;	
	uint8_t reserve[4];
} style_text_t;
