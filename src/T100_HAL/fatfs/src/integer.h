/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#ifndef _FF_INTEGER
#define _FF_INTEGER

#include <stdint.h>

/* These types MUST be 16-bit or 32-bit */
#ifndef INT
typedef int				INT;
#endif /* !INT */

#ifndef UINT
typedef unsigned int	UINT;
#endif /* !UINT */

/* This type MUST be 8-bit */
#ifndef BYTE
typedef unsigned char	BYTE;
#endif /* !BYTE */

/* These types MUST be 16-bit */
#ifndef SHORT
typedef short			SHORT;
#endif /* !SHORT */

#ifndef WORD
typedef unsigned short	WORD;
#endif /* !WORD */

#ifndef WCHAR
typedef unsigned short	WCHAR;
#endif /* !WCHAR */

/* These types MUST be 32-bit */
#ifndef LONG
typedef int32_t			LONG;
#endif /* !LONG */

#ifndef DWORD
typedef uint32_t        DWORD;
#endif /* !DWORD */

/* This type MUST be 64-bit (Remove this for ANSI C (C89) compatibility) */
typedef uint64_t QWORD;

#endif



