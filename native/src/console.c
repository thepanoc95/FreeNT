#include "freent_nt.h"
HANDLE freent_stdout(void) { return freent_current_peb()->ProcessParameters->StandardOutput; }
HANDLE freent_stdin(void) { return freent_current_peb()->ProcessParameters->StandardInput; }
void freent_write(const char *text) { IO_STATUS_BLOCK iosb; ULONG length = 0; while (text[length] != '\0') ++length; (void)freent_ndk_write_file(freent_stdout(), &iosb, (PVOID)text, length); }
void freent_write_u32(ULONG value) { char buffer[10]; ULONG length = 0; do { buffer[length++] = (char)('0' + (value % 10)); value /= 10; } while (value != 0); while (length != 0) { char digit = buffer[--length]; IO_STATUS_BLOCK iosb; (void)freent_ndk_write_file(freent_stdout(), &iosb, &digit, 1); } }
NTSTATUS freent_read_line(char *buffer, ULONG capacity, ULONG *length) { IO_STATUS_BLOCK iosb; NTSTATUS status; ULONG read; if (capacity == 0) return (NTSTATUS)0xC000000DL; status = freent_ndk_read_file(freent_stdin(), &iosb, buffer, capacity - 1); if (!NT_SUCCESS(status)) return status; read = (ULONG)iosb.Information; while (read != 0 && (buffer[read - 1] == '\r' || buffer[read - 1] == '\n')) --read; buffer[read] = '\0'; *length = read; return STATUS_SUCCESS; }
void freent_zero(void *buffer, ULONG length) { volatile UCHAR *bytes = (volatile UCHAR *)buffer; while (length != 0) { *bytes++ = 0; --length; } }
