/* NDK contract functions - thin wrappers around NT API for native userland. */
#include <freent_nt.h>

NTSTATUS freent_ndk_read_file(HANDLE handle, IO_STATUS_BLOCK *iosb, PVOID buffer, ULONG length) { return NtReadFile(handle, 0, 0, 0, iosb, buffer, length, 0, 0); }
NTSTATUS freent_ndk_write_file(HANDLE handle, IO_STATUS_BLOCK *iosb, PVOID buffer, ULONG length) { return NtWriteFile(handle, 0, 0, 0, iosb, buffer, length, 0, 0); }
