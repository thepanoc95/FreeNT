#include "freent_nt.h"
PEB_PARTIAL *freent_current_peb(void) { PEB_PARTIAL *peb; __asm__ volatile ("movq %%gs:0x60, %0" : "=r"(peb)); return peb; }
void freent_entry(void) { freent_dispatch(); NtTerminateProcess((HANDLE)(ULONG_PTR)-1, STATUS_SUCCESS); for (;;) { } }
