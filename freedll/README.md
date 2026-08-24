# FreeNT FreeDLL

**FreeDLL** is a companion DLL to `ntdll.dll` for the FreeNT project. It provides
a **Tiny C Runtime** and NT-compatible API implemented as a freestanding DLL
with no dependencies on the Windows CRT (`ucrtbase.dll`, `msvcrt.dll`) or
`kernel32.dll`/`kernelbase.dll`.

## Purpose

FreeNT executables are freestanding: they import only `ntdll.dll`. FreeDLL
provides the C runtime support (memory operations, string functions, heap
management, process utilities, and RTL functions) so that FreeNT applications
can have a full runtime without depending on the Windows CRT layer.

## Contents

- **`include/freedll.h`** — Main header with NT-compatible types and CRT declarations.
  Fully self-contained; does not include Windows SDK headers.

- **`src/crt_memory.c`** — Tiny CRT memory functions:
  `memcpy`, `memmove`, `memset`, `memcmp`, `memchr`

- **`src/crt_string.c`** — CRT string functions:
  `strlen`, `strnlen`, `strcpy`, `strncpy`, `strcat`, `strncat`,
  `strcmp`, `strncmp`, `strchr`, `strrchr`, `strstr`, `strtok`, `strdup`,
  plus wide string variants (`wcslen`, `wcscmp`, `wcsncmp`, `wcscpy`, `wcsncpy`),
  and standard library functions (`atoi`, `atol`, `atoll`, `abs`, `labs`, `qsort`)

- **`src/crt_format.c`** — CRT format functions:
  `snprintf`, `vsnprintf` — a self-contained printf-style formatter with
  support for `%d`, `%u`, `%x`, `%X`, `%o`, `%s`, `%c`, `%p`, `%n`, `%%`,
  width, precision, and length modifiers.

- **`src/heap.c`** — Heap management:
  `GetProcessHeap`, `CreateHeap`, `DestroyHeap`, `RtlCreateHeap`,
  `RtlDestroyHeap`, `RtlAllocateHeap`, `RtlReAllocateHeap`,
  `RtlFreeHeap`, `RtlSizeHeap`

- **`src/process.c`** — Process/thread utilities:
  `GetCurrentProcess`, `GetCurrentThread`, `GetCurrentProcessId`,
  `GetCurrentThreadId`, `ExitProcess`, `Sleep`, `GetTickCount`,
  `QueryPerformanceCounter`, `QueryPerformanceFrequency`,
  `GetEnvironmentVariableA/W`, `SetEnvironmentVariableA/W`,
  `GetLastError`, `SetLastError`

- **`src/stringconv.c`** — String conversion and module management:
  `RtlMultiByteToUnicodeN`, `RtlUnicodeToMultiByteN`,
  `GetModuleHandleA/W`, `LoadLibraryA`, `GetProcAddress`

- **`src/rtl.c`** — RTL runtime library functions:
  `RtlMoveMemory`, `RtlCopyMemory`, `RtlFillMemory`, `RtlZeroMemory`,
  `RtlCompareMemory`, `RtlCopyUnicodeString`, `RtlDuplicateUnicodeString`,
  `RtlAppendUnicodeStringToString`, `RtlGetVersion`,
  `RtlSystemTimeToLocalTime`, `RtlTimeToTimeFields`,
  `RtlTimeFieldsToTime`, `RtlNtStatusToDosError`,
  `RtlFormatCurrentUserKeyPath`

- **`src/dllmain.c`** — DLL lifecycle management:
  `FreeDllMain`, `DllMain`, TLS callbacks, FLS API (`FlsAlloc`,
  `FlsGetValue`, `FlsSetValue`, `FlsFree`), CRT initialization,
  `atexit`, `__cxa_atexit`, C++ support (`__purecall`),
  `RtlCaptureContext`

- **`freedll.def`** — Module definition file with all exports.

- **`freedll.rc`** — Windows resource file with version information.

## Building

```powershell
# From the FreeNT root directory:
make freedll

# Or from the freedll directory directly:
cd freedll
make
```

### Build Targets

| Target | Description |
|--------|-------------|
| `all` | Build `freedll.dll` |
| `check` | Verify the DLL only imports from `ntdll.dll` |
| `clean` | Remove build artifacts |

## Architecture

FreeDLL is designed to be fully freestanding:

1. **No CRT dependency** — All CRT functions are implemented internally.
2. **Import-only from ntdll.dll** — The DLL only calls into `ntdll.dll`
   for system services (virtual memory allocation, thread termination,
   etc.).
3. **Minimal types** — The header `freedll.h` defines its own NT types
   without relying on the Windows SDK.
4. **Tiny footprint** — The heap manager uses a simple first-fit
   allocator with coalescing to keep code size minimal.

## Compatibility

FreeDLL exports function names compatible with:
- The C runtime (so compiled code using `memcpy` etc. can link)
- Common `kernel32.dll` patterns (via `GetModuleHandle`, `Sleep`, etc.)
- `ntdll.dll` RTL functions (`RtlAllocateHeap`, `RtlCopyMemory`, etc.)

## License

BSD 3-Clause License. See the top-level `LICENSE` file for details.
