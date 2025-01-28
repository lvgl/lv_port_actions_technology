/*!
 * \file      app_utility.h
 * \brief     Utility interface
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#ifndef _LIB_APP_UTILITY_H
#define _LIB_APP_UTILITY_H


#include <stdint.h>
#include <string.h>


/*!
 * \brief Get aligned value
 * \par Sample
 * \code
 *     int  a = 5;
 *     int  b = _ALIGN(a, 4);
 * \endcode
 *     b == 8;
 */
#define _ALIGN(_v, _a)  (((_v) + ((_a) - 1)) / (_a) * (_a))


/*!
 * \brief Get minimum value
 */
#define _MIN(_a, _b)  (((_a) < (_b)) ? (_a) : (_b))


/*!
 * \brief Get maximum value
 */
#define _MAX(_a, _b)  (((_a) > (_b)) ? (_a) : (_b))


/*!
 * \brief Get clip value, _min <= _v <= _max
 * \par Sample
 * \code
 *     int  a = 3;
 *     _CLIP(a, 5, 8);
 * \endcode
 *     a == 5;
 */
#define _CLIP(_v, _min, _max)      \
do                                 \
{                                  \
    if (_v < (_min)) _v = (_min);  \
    if (_v > (_max)) _v = (_max);  \
}                                  \
while (0)


/*!
 * \brief Swap 2 variables
 * \par Sample
 * \code
 *     int  a = 1, b = 2;
 *     _SWAP(&a, &b, int); 
 * \endcode
 *     a == 2; b == 1;
 */
#define _SWAP(_p1, _p2, _type)        \
do                                    \
{                                     \
    _type  _t = *(_type*)(_p1);       \
                                      \
    *(_type*)(_p1) = *(_type*)(_p2);  \
    *(_type*)(_p2) = _t;              \
}                                     \
while (0)

#define _GET_BUF_U8(_buf, _pos, _var)    \
do                                       \
{                                        \
    u8_t* _p = (u8_t*)(_buf) + (_pos);   \
                                         \
    _var = (u8_t)(_p[0]);                \
    _pos += 1;                           \
}                                        \
while (0)


#define _GET_BUF_U16(_buf, _pos, _var)     \
do                                         \
{                                          \
    u8_t* _p = (u8_t*)(_buf) + (_pos);     \
                                           \
    _var = (u16_t)(_p[0] | (_p[1] << 8));  \
    _pos += 2;                             \
}                                          \
while (0)


#define _GET_BUF_U32(_buf, _pos, _var)                               \
do                                                                   \
{                                                                    \
    u8_t* _p = (u8_t*)(_buf) + (_pos);                               \
                                                                     \
    _var = (u32_t)(_p[0] | (_p[1]<<8) | (_p[2]<<16) | (_p[3]<<24));  \
    _pos += 4;                                                       \
}                                                                    \
while (0)


#define _GET_BUF_DATA(_buf, _pos, _data, _len)    \
do                                                \
{                                                 \
    memcpy(_data, (u8_t*)(_buf) + (_pos), _len);  \
    _pos += (_len);                               \
}                                                 \
while (0)


#define _PUT_BUF_U8(_buf, _pos, _val)       \
do                                          \
{                                           \
    if ((_buf) != NULL)                     \
    {                                       \
        u8_t* _p = (u8_t*)(_buf) + (_pos);  \
        _p[0] = (u8_t)(_val);               \
    }                                       \
    _pos += 1;                              \
}                                           \
while (0)


#define _PUT_BUF_U16(_buf, _pos, _val)      \
do                                          \
{                                           \
    if ((_buf) != NULL)                     \
    {                                       \
        u8_t* _p = (u8_t*)(_buf) + (_pos);  \
                                            \
        _p[0] = (u8_t)((_val) & 0xff);      \
        _p[1] = (u8_t)((_val) >> 8);        \
    }                                       \
    _pos += 2;                              \
}                                           \
while (0)


#define _PUT_BUF_U32(_buf, _pos, _val)      \
do                                          \
{                                           \
    if ((_buf) != NULL)                     \
    {                                       \
        u8_t* _p = (u8_t*)(_buf) + (_pos);  \
                                            \
        _p[0] = (u8_t)((_val) & 0xff);      \
        _p[1] = (u8_t)((_val) >> 8);        \
        _p[2] = (u8_t)((_val) >> 16);       \
        _p[3] = (u8_t)((_val) >> 24);       \
    }                                       \
    _pos += 4;                              \
}                                           \
while (0)


#define _PUT_BUF_DATA(_buf, _pos, _data, _len)        \
do                                                    \
{                                                     \
    if ((_buf) != NULL)                               \
    {                                                 \
        memcpy((u8_t*)(_buf) + (_pos), _data, _len);  \
    }                                                 \
    _pos += (_len);                                   \
}                                                     \
while (0)


#endif  // _LIB_APP_UTILITY_H


