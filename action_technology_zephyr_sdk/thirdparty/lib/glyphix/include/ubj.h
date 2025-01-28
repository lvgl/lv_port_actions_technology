/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __UBJ_H__
#define __UBJ_H__

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UBJ_INT32_MAX            2147483647
#define UBJ_INT32_MIN          (~0x7fffffff)   /* -2147483648 is unsigned */

#define UBJ_INT8_MAX                    127
#define UBJ_INT16_MAX                 32767

#define UBJ_OK              0
#define UBJ_ERROR           1       // General error
#define UBJ_NOMEM_ERROR     2       // insufficient memory
#define UBJ_PARAM_ERROR     3       // Parameter error
#define UBJ_PARSE_ERROR     4       // Parsing error
#define UBJ_WRITE_ERROR     5       // write error
#define UBJ_READ_ERROR      6       // Read error
#define UBJ_UNKNOWN_ERROR  -1,      // Occupancy, compulsory ubj_err_t is a signed type

typedef int ubj_err_t;

typedef enum
{
    UBJ_TYPE_MIXED = 0,             // Mixed type for containers
    UBJ_TYPE_NULL,                  // NULL
    UBJ_TYPE_NOOP,                  // NOOP
    UBJ_TYPE_TRUE,                  // TRUE
    UBJ_TYPE_FALSE,                 // FALSE
    UBJ_TYPE_CHAR,                  // CHAR
    UBJ_TYPE_STRING,                // STRING
    UBJ_TYPE_HIGH_PRECISION,        // PRECISION(STRING)
    UBJ_TYPE_INT8,                  // INTEGER
    UBJ_TYPE_UINT8,                 // UINT8
    UBJ_TYPE_INT16,                 // INT16
    UBJ_TYPE_INT32,                 // INT32
    UBJ_TYPE_INT64,                 // INT64
    UBJ_TYPE_FLOAT32,               // FLOAT
    UBJ_TYPE_FLOAT64,               // DOUBLE
    UBJ_TYPE_ARRAY,                 // ARRAY
    UBJ_TYPE_OBJECT,                // OBJECT
    UBJ_NUM_TYPES                   // This is not a type, it only represents the number of types
} ubj_type_t;

#define UBJ_CONTAINER_IS_SIZED      0x1     /* Size properties */
#define UBJ_CONTAINER_IS_TYPED      0x2     /* Type properties */
#define UBJ_CONTAINER_IS_ARRAY      0x4     /* Array properties */
#define UBJ_CONTAINER_IS_OBJECT     0x8     /* Object properties */
#define UBJ_CONTAINER_EXPECTS_KEY   0x10    /* Keyword properties */
#define UBJ_CONTAINER_IGNORE_FLAG   0x20    /* Bare data attribute */
#define UBJ_CONTAINER_BUFFER_FLAG   0x40    /* Buffer properties */

#define UBJ_CONTAINER_IS_GENERAL    (UBJ_CONTAINER_IS_SIZED | \
                                     UBJ_CONTAINER_IS_TYPED | \
                                     UBJ_CONTAINER_IS_ARRAY | \
                                     UBJ_CONTAINER_IS_OBJECT)

#define UBJ_CONTAINER_IS_BUFFER     (UBJ_CONTAINER_IS_ARRAY | \
                                     UBJ_CONTAINER_IS_SIZED | \
                                     UBJ_CONTAINER_IS_TYPED | \
                                     UBJ_CONTAINER_BUFFER_FLAG)

struct ubjson_container
{
    uint8_t flags;          // Container related marking
    int8_t type;            // Type of container
    int32_t remaining;      // Total container size
};

#ifndef UBJ_CONTAINER_STACK_DEPTH
#define UBJ_CONTAINER_STACK_DEPTH   16
#endif

typedef struct ubjsonw_object ubjsonw_t;
typedef struct ubjsonr_object ubjsonr_t;

/*
 * TODO:
 *     Distinguish between ordinary error and unrecoverable error.
 *     Unrecoverable error occurs and all operations are invalid
*/

struct ubjsonw_object
{
    /* Write callback. When the data pointer is null, it means that a 0 is written */
    size_t (*write)(ubjsonw_t *ubj, const void *data, size_t size, void *userdata);
    void (*close)(ubjsonw_t *ubj, void *userdata);   // close callback
    void *userdata;                     // uasr data
    size_t total;                       // write total
    struct ubjson_container stack[UBJ_CONTAINER_STACK_DEPTH];
    int16_t stack_depth;
    int16_t stack_point;
    ubj_err_t error;
};

#define UBJ_ITERATOR_SKIP_SUBOBJECT  0x2
#define UBJ_ITERATOR_PICK_VALID      0x4
#define UBJ_ITERATOR_READ_END        0x8

struct ubjson_iterator
{
    size_t promise_pos;                     // Expected starting position
    uint8_t head_value;                     // Record the value currently read
    uint8_t flags;                          // Mark, whether to access sub elements, whether pick is valid
};

#define UBJ_CONTAINER_ENTER_FLAG   0x01    /* Enter container */
#define UBJ_CONTAINER_EXIT_FLAG    0x02    /* Exit container */
#define UBJ_CONTAINER_ACCESS_FLAG  0x03    /* Access container */

#define UBJ_SIZE_INVALID           ((size_t)(-1))

typedef struct ubjson_buf
{
    void *ptr;
    size_t count;
}ubj_buf_t;

typedef struct ubjson_str
{
    char *ptr;
    size_t size;
}ubj_str_t;

typedef struct
{
    ubj_type_t type;
    ubj_err_t error;                        /* error code */
    uint8_t flags;                          /* Mark whether it is currently in a container, 
                                               whether it is a strong type, fixed size container */
    struct
    {
        ubj_type_t type;                    /* Type object container, array container */
        uint8_t act;                        /* Enter container, iterate container, exit container */
        int16_t level;                      /* Hierarchy of containers */
        int32_t count;                      /* Number of unreachable elements in container */
        int32_t keylen;                     /* Key string length */
        char *key;                          /* key */
    } container;
    union
    {
        char vchar;
        uint8_t vbool;
        uint8_t vuint8;
        int8_t vint8;
        int16_t vint16;
        int32_t vint32;
        float vfloat;
        double vdouble;
        int64_t vint64;
        struct
        {
            char *string;
            size_t size;
        } vstring;
        struct
        {
            char *hpstring;
            size_t size;
        }vhp;
    } value;
} ubj_value_t;

struct _ubjsonr_mem
{
    ubj_value_t value;
    ubj_buf_t buf[2];               // 0: key buf  1: string buf
#define _UBJSONR_MEM_KEY    0
#define _UBJSONR_MEM_STRING 1
};

struct ubjsonr_object
{
    /* Read callback. When the data pointer is null, it means that it is offset by a length */
    size_t (*read)(ubjsonr_t *ubj, void *data, size_t size, void *userdata);
    void (*close)(ubjsonr_t *ubj, void *userdata);   // close callback
    void *userdata;                     // user data
    size_t total;                       // read total
    struct ubjson_iterator iterator;    // iterator
    struct _ubjsonr_mem mem;            // read cache
    struct ubjson_container stack[UBJ_CONTAINER_STACK_DEPTH];
    int16_t stack_depth;
    int16_t stack_point;
    ubj_err_t error;                    // error code
};

/* object write API */
ubjsonw_t *ubj_write_init(ubjsonw_t* ubj,
    size_t (*write)(ubjsonw_t *, const void *, size_t, void *),
    void (*close)(ubjsonw_t *, void *),
    void *userdata);
size_t ubj_write_end(ubjsonw_t* ubj);

/* container API */
ubj_err_t ubj_begin_array(ubjsonw_t *ubj);
ubj_err_t ubj_begin_fixed_array(ubjsonw_t *ubj, ubj_type_t type, long count);
ubj_err_t ubj_end_array(ubjsonw_t *ubj);
ubj_err_t ubj_begin_object(ubjsonw_t *ubj);
ubj_err_t ubj_begin_fixed_object(ubjsonw_t *ubj, ubj_type_t type, long count);
ubj_err_t ubj_end_object(ubjsonw_t *ubj);
ubj_err_t ubj_write_key(ubjsonw_t *ubj, const char *key);

/* Fill data API */
ubj_err_t ubj_write_string(ubjsonw_t *ubj, const char *v);
ubj_err_t ubj_write_fixed_string(ubjsonw_t *ubj, const char *v, int len);
ubj_err_t ubj_write_char(ubjsonw_t *ubj, char v);
ubj_err_t ubj_write_uint8(ubjsonw_t *ubj, uint8_t v);
ubj_err_t ubj_write_int8(ubjsonw_t *ubj, int8_t v);
ubj_err_t ubj_write_int16(ubjsonw_t *ubj, int16_t v);
ubj_err_t ubj_write_int32(ubjsonw_t *ubj, int32_t v);
ubj_err_t ubj_write_int64(ubjsonw_t *ubj, int64_t v);
ubj_err_t ubj_write_high_precision(ubjsonw_t *ubj, const char *hp);
ubj_err_t ubj_write_integer(ubjsonw_t *ubj, int64_t v);

ubj_err_t ubj_write_float32(ubjsonw_t *ubj, float v);
ubj_err_t ubj_write_float64(ubjsonw_t *ubj, double v);

ubj_err_t ubj_write_noop(ubjsonw_t *ubj);
ubj_err_t ubj_write_null(ubjsonw_t *ubj);
ubj_err_t ubj_write_true(ubjsonw_t *ubj);
ubj_err_t ubj_write_false(ubjsonw_t *ubj);

ubj_err_t ubj_write_ubjson(ubjsonw_t *ubj, const uint8_t *ubjson, size_t len);
ubj_err_t ubj_object_write_ubjson(ubjsonw_t *ubj, const char *key, const uint8_t *ubjson, size_t len);

/* write key and value */
ubj_err_t ubj_object_write_string(ubjsonw_t *ubj, const char *key, const char *v);
ubj_err_t ubj_object_write_fixed_string(ubjsonw_t *ubj, const char *key, const char *v, int len);
ubj_err_t ubj_object_write_char(ubjsonw_t *ubj, const char *key, char v);
ubj_err_t ubj_object_write_uint8(ubjsonw_t *ubj, const char *key, uint8_t v);
ubj_err_t ubj_object_write_int8(ubjsonw_t *ubj, const char *key, int8_t v);
ubj_err_t ubj_object_write_int16(ubjsonw_t *ubj, const char *key, int16_t v);
ubj_err_t ubj_object_write_int32(ubjsonw_t *ubj, const char *key, int32_t v);
ubj_err_t ubj_object_write_int64(ubjsonw_t *ubj, const char *key, int64_t v);
ubj_err_t ubj_object_write_high_precision(ubjsonw_t *ubj, const char *key, const char *hp);
ubj_err_t ubj_object_write_integer(ubjsonw_t *ubj, const char *key, int64_t v);

ubj_err_t ubj_object_write_float32(ubjsonw_t *ubj, const char *key, float v);
ubj_err_t ubj_object_write_float64(ubjsonw_t *ubj, const char *key, double v);

ubj_err_t ubj_object_write_null(ubjsonw_t *ubj, const char *key);
ubj_err_t ubj_object_write_true(ubjsonw_t *ubj, const char *key);
ubj_err_t ubj_object_write_false(ubjsonw_t *ubj, const char *key);

ubj_err_t ubj_object_write_object(ubjsonw_t *ubj, const char *key);
ubj_err_t ubj_object_write_array(ubjsonw_t *ubj, const char *key);
ubj_err_t ubj_object_write_fixed_object(ubjsonw_t *ubj, const char *key, ubj_type_t type, long count);
ubj_err_t ubj_object_write_fixed_array(ubjsonw_t *ubj, const char *key, ubj_type_t type, long count);

/* write buf */
ubj_err_t ubj_write_buffer(ubjsonw_t *ubj, const uint8_t *data, ubj_type_t type, size_t count);
ubj_err_t ubj_write_arraybuffer(ubjsonw_t* ubj, const uint8_t* data, size_t count);
ubj_err_t ubj_object_write_buffer(ubjsonw_t *ubj, const char *key, const uint8_t *data, ubj_type_t type, size_t count);
ubj_err_t ubj_object_write_arraybuffer(ubjsonw_t* ubj, const char* key, const uint8_t* data, size_t count);
/* ubj read API */
ubjsonr_t *ubj_read_init(ubjsonr_t* ubj, 
    size_t (*read)(ubjsonr_t *, void *, size_t, void *),
    void (*close)(ubjsonr_t *, void *),
    void *userdata);
const ubj_value_t *ubj_read_next(ubjsonr_t *ubj);
size_t ubj_read_end(ubjsonr_t* ubj);
ubj_err_t ubj_skip_subobject(ubjsonr_t* ubj);         /* Skip child elements */
int ubj_isvalid(ubjsonr_t* ubj);

/* ubj read helper API */
typedef struct ubjsonr_helper_object
{
    struct ubjsonr_helper_object *next;
    int8_t type;
    uint8_t flags;
    union
    {
        char *key;
        int index;
    } auxiliary;
    union
    {
        char vchar;
        uint8_t vbool;
        uint8_t vuint8;
        int8_t vint8;
        int16_t vint16;
        int32_t vint32;
        float vfloat;
        double vdouble;
        int64_t vint64;
        ubj_str_t vstring;
        ubj_str_t vhp;
        ubj_buf_t vbuf;
        struct ubjsonr_helper_object *vchild;
    } value;
} ubjh_t;

ubjh_t *ubj_parse(const void *buff, size_t len);
void ubj_parse_free(ubjh_t *ubj);
ubj_type_t ubj_item_type(const ubjh_t *ubj);
int ubj_array_length(const ubjh_t *ubj);
int ubj_buffer_length(const ubjh_t *ubj);
ubjh_t *ubj_array_item(const ubjh_t *ubj, int index);
ubjh_t *ubj_object_item(const ubjh_t *ubj, const char *key);
ubjh_t *ubj_next_item(const ubjh_t *ubj);
ubjh_t *ubj_child_item(const ubjh_t *ubj);
ubj_err_t ubj_dumpb(const ubjh_t *ubj, ubjsonw_t *ubj_write);
ubjh_t *ubj_deepcopy(const ubjh_t *ubj);

/* Check item type and return its value */
char ubj_get_char(const ubjh_t *ubj);
uint8_t ubj_get_bool(const ubjh_t *ubj);
uint8_t ubj_get_uint8(const ubjh_t *ubj);
int8_t ubj_get_int8(const ubjh_t *ubj);
int16_t ubj_get_int16(const ubjh_t *ubj);
int32_t ubj_get_int32(const ubjh_t *ubj);
float ubj_get_float(const ubjh_t *ubj);
double ubj_get_double(const ubjh_t *ubj);
int64_t ubj_get_int64(const ubjh_t *ubj);
int64_t ubj_get_integer(const ubjh_t *ubj);
ubj_str_t ubj_get_string(const ubjh_t *ubj);
ubj_str_t ubj_get_hpstring(const ubjh_t *ubj);
ubj_buf_t ubj_get_buffer(const ubjh_t *ubj);

/* Create item */
ubjh_t *ubj_create_string(const char *v);
ubjh_t *ubj_create_char(char v);
ubjh_t *ubj_create_uint8(uint8_t v);
ubjh_t *ubj_create_int8(int8_t v);
ubjh_t *ubj_create_int16(int16_t v);
ubjh_t *ubj_create_int32(int32_t v);
ubjh_t *ubj_create_int64(int64_t v);
ubjh_t *ubj_create_high_precision(const char *hp);
ubjh_t *ubj_create_integer(int64_t v);
ubjh_t *ubj_create_float32(float v);
ubjh_t *ubj_create_float64(double v);
ubjh_t *ubj_create_noop(void);
ubjh_t *ubj_create_null(void);
ubjh_t *ubj_create_true(void);
ubjh_t *ubj_create_false(void);
ubjh_t *ubj_create_array(void);
ubjh_t *ubj_create_object(void);
ubjh_t *ubj_create_buffer(const uint8_t *data, ubj_type_t type, size_t count);

/* Add item */
ubjh_t *ubj_add_item_to_array(ubjh_t *ubj_array, ubjh_t *item, long index);
ubjh_t *ubj_add_item_to_object(ubjh_t *ubj_object, const char *key, ubjh_t *item);

/* These functions check the type of an item */
int ubj_is_null(const ubjh_t *ubj);
int ubj_is_noop(const ubjh_t *ubj);
int ubj_is_char(const ubjh_t *ubj);
int ubj_is_true(const ubjh_t *ubj);
int ubj_is_false(const ubjh_t *ubj);
int ubj_is_bool(const ubjh_t *ubj);
int ubj_is_uint8(const ubjh_t *ubj);
int ubj_is_int8(const ubjh_t *ubj);
int ubj_is_int16(const ubjh_t *ubj);
int ubj_is_int32(const ubjh_t *ubj);
int ubj_is_float(const ubjh_t *ubj);
int ubj_is_double(const ubjh_t *ubj);
int ubj_is_int64(const ubjh_t *ubj);
int ubj_is_integer(const ubjh_t *ubj);
int ubj_is_string(const ubjh_t *ubj);
int ubj_is_hpstring(const ubjh_t *ubj);
int ubj_is_buffer(const ubjh_t *ubj);
int ubj_is_object(const ubjh_t *ubj);
int ubj_is_array(const ubjh_t *ubj);

/* make */
#define UBJ_STATIC_MEM_NULL                 (1)                        // NULL
#define UBJ_STATIC_MEM_NOOP                 (1)                        // NOOP
#define UBJ_STATIC_MEM_TRUE                 (1)                        // TRUE
#define UBJ_STATIC_MEM_FALSE                (1)                        // FALSE
#define UBJ_STATIC_MEM_BOOL                 (1)                        // TRUE or FALSE
#define UBJ_STATIC_MEM_CHAR                 (1 + sizeof(char))         // CHAR
#define UBJ_STATIC_MEM_INT8                 (1 + sizeof(int8_t))       // INTEGER
#define UBJ_STATIC_MEM_UINT8                (1 + sizeof(uint8_t))      // UINT8
#define UBJ_STATIC_MEM_INT16                (1 + sizeof(int16_t))      // UINT16
#define UBJ_STATIC_MEM_INT32                (1 + sizeof(int32_t))      // UINT32
#define UBJ_STATIC_MEM_INT64                (1 + sizeof(int64_t))      // UINT64
#define UBJ_STATIC_MEM_INTEGER(_i)          ((_i) > UBJ_INT16_MAX ? UBJ_STATIC_MEM_INT32 : \
                                             (_i) > UBJ_INT8_MAX ? UBJ_STATIC_MEM_INT16 : UBJ_STATIC_MEM_INT8)
#define UBJ_STATIC_MEM_FLOAT32              (1 + sizeof(float))       // FLOAT
#define UBJ_STATIC_MEM_FLOAT64              (1 + sizeof(double))      // DOUBLE
#define UBJ_STATIC_MEM_STRING(_l)           (1 + UBJ_STATIC_MEM_INTEGER(_l) + (_l))  // STRING
#define UBJ_STATIC_MEM_BUFFER(_l)           (4 + UBJ_STATIC_MEM_INTEGER(_l) + (_l))  // BUFFER

uint8_t *ubj_make_null(uint8_t *buf);
uint8_t *ubj_make_noop(uint8_t *buf);
uint8_t *ubj_make_char(uint8_t *buf, char v);
uint8_t *ubj_make_true(uint8_t *buf);
uint8_t *ubj_make_false(uint8_t *buf);
uint8_t *ubj_make_bool(uint8_t *buf, int v);
uint8_t *ubj_make_uint8(uint8_t *buf, uint8_t v);
uint8_t *ubj_make_int8(uint8_t *buf, int8_t v);
uint8_t *ubj_make_int16(uint8_t *buf, int16_t v);
uint8_t *ubj_make_int32(uint8_t *buf, int32_t v);
uint8_t *ubj_make_float(uint8_t *buf, float v);
uint8_t *ubj_make_double(uint8_t *buf, double v);
uint8_t *ubj_make_int64(uint8_t *buf, int64_t v);
uint8_t *ubj_make_integer(uint8_t *buf, int64_t v);
uint8_t *ubj_make_string(uint8_t *buf, const char *s, size_t len);
uint8_t *ubj_make_buffer(uint8_t *buf, uint8_t *data, size_t len);

int ubjson_to_null(const uint8_t *ubjson, size_t len, int def);
int ubjson_to_noop(const uint8_t *ubjson, size_t len, int def);
char ubjson_to_char(const uint8_t *ubjson, size_t len, char def);
int ubjson_to_true(const uint8_t *ubjson, size_t len, int def);
int ubjson_to_false(const uint8_t *ubjson, size_t len, int def);
int ubjson_to_bool(const uint8_t *ubjson, size_t len, int def);
uint8_t ubjson_to_uint8(const uint8_t *ubjson, size_t len, uint8_t def);
int8_t ubjson_to_int8(const uint8_t *ubjson, size_t len, int8_t def);
int16_t ubjson_to_int16(const uint8_t *ubjson, size_t len, int16_t def);
int32_t ubjson_to_int32(const uint8_t *ubjson, size_t len, int32_t def);
float ubjson_to_float(const uint8_t *ubjson, size_t len, float def);
double ubjson_to_double(const uint8_t *ubjson, size_t len, double def);
int64_t ubjson_to_int64(const uint8_t *ubjson, size_t len, int64_t def);
int64_t ubjson_to_integer(const uint8_t *ubjson, size_t len, int64_t def);
char *ubjson_to_string(const uint8_t *ubjson, size_t len, char *out_buf, size_t *out_len);
uint8_t *ubjson_to_buffer(const uint8_t *ubjson, size_t len, uint8_t *out_buf, size_t *out_len);
char *ubjson_to_json(const uint8_t *ubjson, size_t len, char *out_buf, size_t *out_len);

/* port */
void* ubj_malloc(unsigned int size);
void* ubj_realloc(void* rmem, unsigned int newsize);
void ubj_free(void* rmem);
int ubj_printf(const char* format, ...);

/* other API */
ubjsonr_t *ubj_read_memory(const void *buff, size_t len);
ubjsonr_t *ubj_read_static_memory(const void *buff, size_t len);
ubjsonw_t *ubj_write_memory(void);
uint8_t* ubj_get_memory_and_close(ubjsonw_t* ubj);
uint8_t *ubj_get_memory(ubjsonw_t *ubj);
ubjsonw_t *ubj_write_file(const char *path);
ubjsonr_t *ubj_read_file(const char *path);
ubj_err_t ubj_to_json(ubjsonr_t *ubj, char **buff, size_t *len); /* UBJSON to JSON */
ubj_err_t json_to_ubj(ubjsonw_t *ubj, const char *json, size_t len);   /* JSON to UBJSON */

#ifdef __cplusplus
}
#endif
#endif
