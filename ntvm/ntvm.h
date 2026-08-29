#ifndef FREENT_NTVM_H
#define FREENT_NTVM_H

#include "freedll.h"
#define NTVM_REAL_MODE_MEMORY (1024U * 1024U)
#define NTVM_COM_LOAD_OFFSET 0x100U
#define NTVM_COM_SEGMENT     0x1000U

typedef struct _NTVM_CPU {
    USHORT ax, bx, cx, dx, si, di, bp, sp;
    USHORT cs, ds, es, ss, ip, flags;
} NTVM_CPU;
typedef struct _NTVM {
    UCHAR *memory;
    SIZE_T memory_size;
    NTVM_CPU cpu;
} NTVM;

NTSTATUS NtvmInitialize(NTVM *vm);
NTSTATUS NtvmLoadCom(NTVM *vm, const void *image, SIZE_T image_size);
void NtvmDestroy(NTVM *vm);
#endif
