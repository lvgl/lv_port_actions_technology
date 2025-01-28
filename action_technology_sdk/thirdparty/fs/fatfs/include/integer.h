/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#ifndef _FF_INTEGER
#define _FF_INTEGER

#ifdef _WIN32	/* FatFs development platform */

#include <windows.h>
#include <tchar.h>
typedef unsigned __int64 QWORD;


#else			/* Embedded platform */

/* These types MUST be 16-bit or 32-bit */
#ifndef INT
typedef int				INT;
#endif
#ifndef UINT
typedef unsigned int	UINT;
#endif

/* This type MUST be 8-bit */
#ifndef BYTE
typedef unsigned char	BYTE;
#endif

/* These types MUST be 16-bit */
#ifndef SHORT
typedef short			SHORT;
#endif
#ifndef WORD
typedef unsigned short	WORD;
#endif
#ifndef WCHAR
typedef unsigned short	WCHAR;
#endif

/* These types MUST be 32-bit */
#ifndef LONG
typedef long			LONG;
#endif
#ifndef DWORD
typedef unsigned long	DWORD;
#endif

/* This type MUST be 64-bit (Remove this for C89 compatibility) */
#ifndef QWORD
typedef unsigned long long QWORD;
#endif

#endif

#endif
