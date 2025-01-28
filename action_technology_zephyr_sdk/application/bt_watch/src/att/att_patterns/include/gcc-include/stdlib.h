#ifndef _STDLIB_H_
#define _STDLIB_H_


#include <types.h>
#include <ctype.h>


/*!
 * \brief get absolute value of integer
 */
static inline int abs(int x)
{
    if (x < 0)
        return -(x);
    else
        return x;
}

/*!
 * \brief Convert a string to an long integer
 */
extern long strtol(const char* str, char** endptr, unsigned int base);

/*!
 * \brief Convert a string to an integer
 */
extern int atoi(const char* str);

extern void *malloc(size_t size);

extern void free(void *ptr);

#endif  /* _STDLIB_H_ */

