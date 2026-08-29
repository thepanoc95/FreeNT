#include "ntvm.h"

NTSTATUS NtvmInitialize(NTVM *vm)
{
    if (!vm) return STATUS_INVALID_PARAMETER;
    freent_memset(vm, 0, sizeof(*vm));
    vm->memory = freent_calloc(1, NTVM_REAL_MODE_MEMORY);
    if (!vm->memory) return STATUS_NO_MEMORY;
    vm->memory_size = NTVM_REAL_MODE_MEMORY;
    return STATUS_SUCCESS;
}

NTSTATUS NtvmLoadCom(NTVM *vm, const void *image, SIZE_T image_size)
{
    SIZE_T base;
    if (!vm || !vm->memory || !image || !image_size) return STATUS_INVALID_PARAMETER;
    base = ((SIZE_T)NTVM_COM_SEGMENT << 4) + NTVM_COM_LOAD_OFFSET;
    if (image_size > 0xFF00U || base + image_size > vm->memory_size)
        return STATUS_BUFFER_TOO_SMALL;
    freent_memset(vm->memory + ((SIZE_T)NTVM_COM_SEGMENT << 4), 0, 0x10000U);
    freent_memcpy(vm->memory + base, image, image_size);
    vm->cpu.cs = vm->cpu.ds = vm->cpu.es = vm->cpu.ss = NTVM_COM_SEGMENT;
    vm->cpu.ip = NTVM_COM_LOAD_OFFSET;
    vm->cpu.sp = 0xFFFEU;
    vm->cpu.flags = 0x0202U;
    return STATUS_SUCCESS;
}

void NtvmDestroy(NTVM *vm)
{
    if (!vm) return;
    freent_free(vm->memory);
    freent_memset(vm, 0, sizeof(*vm));
}
