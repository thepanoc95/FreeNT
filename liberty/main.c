#include <windows.h>
#include <winternl.h>

typedef NTSTATUS(NTAPI* pfnLdrLoadDll)(
    PWSTR DllPath,
    PULONG DllCharacteristics,
    PUNICODE_STRING DllName,
    PVOID* DllHandle
);

/*
NTSTATUS LoadPOSIXSubsystemLayer() {
    pfnLdrLoadDll LdrLoadDll = (pfnLdrLoadDll)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrLoadDll"); 
    UNICODE_STRING dllName;
    RtlInitUnicodeString(&dllName, L"\\SystemRoot\\System32\\sysposix.dll");
    PVOID dllHandle = NULL;
    ULONG flags = 0;
    NTSTATUS status = LdrLoadDll(NULL, &flags, &dllName, &dllHandle);
    return status;
}
*/

NTSTATUS LoadNTDYSubsystem() {
    pfnLdrLoadDll LdrLoadDll = (pfnLdrLoadDll)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrLoadDll");
    UNICODE_STRING dllName;
    RtlInitUnicodeString(&dllName, L"\\SystemRoot\\System32\\ntdylib.dll");
    PVOID dllHandle = NULL;
    ULONG flags = 0;
    NTSTATUS status = LdrLoadDll(NULL, &flags, &dllName, &dllHandle);
    return status;
}

int main(void) {
    NTSTATUS status;
    status = LoadNTDYSubsystem();
    if (status != 0) {
        return 1;
    }
    return 0;
}