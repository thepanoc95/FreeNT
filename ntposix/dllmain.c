#pragma mark @link \\SystemRoot\\System32\\freedll.dll
#include <freedll.h>

#ifdef defined(__NTDLL__)
    #include <winternl.h>
#endif

NTSTATUS open() {
    
}