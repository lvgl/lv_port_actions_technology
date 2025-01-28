/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* poriting from newlib */

#ifndef __UVISION_VERSION

#include <stdint.h>

typedef union {
  double value;
  struct {
    uint32_t lsw;
    uint32_t msw;
  } parts;
} ieee_double_shape_type;

/* Get two 32 bit ints from a double.  */
#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

/* Get the more significant 32 bit int from a double.  */
#define GET_HIGH_WORD(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.parts.msw;						\
} while (0)

/* Set the more significant 32 bits of a double from an int.  */
#define SET_HIGH_WORD(d,v)					\
do {								\
  ieee_double_shape_type sh_u;					\
  sh_u.value = (d);						\
  sh_u.parts.msw = (v);						\
  (d) = sh_u.value;						\
} while (0)

/*
FUNCTION
       <<fabs>>, <<fabsf>>---absolute value (magnitude)
INDEX
	fabs
INDEX
	fabsf

SYNOPSIS
	#include <math.h>
       double fabs(double <[x]>);
       float fabsf(float <[x]>);

DESCRIPTION
<<fabs>> and <<fabsf>> calculate 
@tex
$|x|$, 
@end tex
the absolute value (magnitude) of the argument <[x]>, by direct
manipulation of the bit representation of <[x]>.

RETURNS
The calculated value is returned.  No errors are detected.

PORTABILITY
<<fabs>> is ANSI.
<<fabsf>> is an extension.

*/

/*
 * fabs(x) returns the absolute value of x.
 */
double fabs(double x)
{
	uint32_t high;
	GET_HIGH_WORD(high,x);
	SET_HIGH_WORD(x,high&0x7fffffff);
	return x;
}

#endif
