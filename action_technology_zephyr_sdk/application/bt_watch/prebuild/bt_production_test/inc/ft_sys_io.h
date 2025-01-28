#ifndef __FT_SYS_IO_H__
#define __FT_SYS_IO_H__


#include <sys_io.h>


#define IO_WRITEU32(reg,val)            *(REG32)(reg) =  (val)

#define IO_WRITEU16(reg,val)            *(REG16)(reg) =  (val)

#define IO_WRITEU8(reg,val)             *(REG8)(reg)  =  (val)

#define IO_OR_U32(reg,val)              *(REG32)(reg) |= (val)

#define IO_OR_U16(reg,val)              *(REG16)(reg) |= (val)

#define IO_OR_U8(reg,val)               *(REG8)(reg)  |= (val)

#define IO_AND_U32(reg,val)             *(REG32)(reg) &= (val)

#define IO_AND_U16(reg,val)             *(REG16)(reg) &= (val)

#define IO_AND_U8(reg,val)              *(REG8)(reg)  &= (val)
#define IO_ADD(reg,val)                 *(REG32)(reg) += (val)
#define IO_SUB(reg,val)                 *(REG32)(reg) -= (val)


#define IO_READU32(reg)                 (*(REG32)(reg))
#define IO_READU16(reg)                 (*(REG16)(reg))
#define IO_READU8(reg)                  (*(REG8)(reg))

#define IO_SET_MSK_U32(reg,mask,val)    (*(REG32)(reg)) = (((*(REG32)(reg))&(~(mask)))|((mask)&(val)))

#define IO_READ(reg)                    IO_READU32(reg)
#define IO_WRITE(reg,val)               IO_WRITEU32(reg,val)
#define IO_OR(reg,val)                  IO_OR_U32(reg,val)
#define IO_AND(reg,val)                 IO_AND_U32(reg,val)
#define IO_SET_MASK(reg,mask,val)       IO_SET_MSK_U32(reg,mask,val)

#endif /* __FT_SYS_IO_H__ */
