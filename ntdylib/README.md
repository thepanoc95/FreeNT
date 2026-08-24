# FreeNT NTDYLIB

**NTDYLIB** is a Dynamic Library Loader for NT-based systems, implemented as
a Windows DLL. It loads and executes PE (Portable Executable) format DLLs
using only NT Native API calls through [FreeDLL](freedll).

## Purpose

NTDYLIB provides an alternative to the Windows `loader` that is compatible
with the FreeNT ecosystem. It implements:
- PE file parsing (DOS, NT, section headers)
- Virtual memory allocation for DLL images
- Base relocation processing
- Import table resolution
- Export table lookup
- DLL lifecycle management (DllMain calls, TLS callbacks)

## Architecture

NTDYLIB has a two-layer architecture:

```
┌─────────────────────────────────────────────┐
│            ntdylib.dll (this project)       │
│                                             │
│  ┌──────────────┐ ┌──────────────┐        │
│  │   loader.c   │ │  exports.c   │        │
│  │  PE parsing,  │ │  Export &    │        │
│  │  relocations, │ │  import       │        │
│  │  mapping      │ │  resolution   │        │
│  └──────────────┘ └──────────────┘        │
│  ┌─────────────────────────────────────┐   │
│  │         dllmain.c                   │   │
│  │  DLL lifecycle, initialization     │   │
│  └─────────────────────────────────────┘   │
└──────────────┬─────────────────────────────┘
               │ uses services from
               ▼
┌─────────────────────────────────────────────┐
│            freedll.dll                      │
│  Tiny C Runtime + NT Syscall Interface      │
│  (memory, strings, heap, ntdll imports)     │
└─────────────────────────────────────────────┘
```

## Building

```powershell
# From the FreeNT root directory:
make ntdylib

# Or from the ntdylib directory directly:
cd ntdylib
make
```

### Build Targets

| Target | Description |
|--------|-------------|
| `all` | Build `ntdylib.dll` |
| `check` | Verify the DLL only imports from `ntdll.dll` and `freedll.dll` |
| `clean` | Remove build artifacts |

## API Reference

### NtdylibInit
Initializes the module loader. Must be called before any other NTDYLIB function.

### NtdylibLoadDll
Loads a PE DLL from disk into the current process. Resolves imports,
applies relocations, and calls `DllMain` with `DLL_PROCESS_ATTACH`.

### NtdylibGetProcAddress
Looks up an exported function by name or ordinal within a loaded module.
Uses the PE export directory for resolution.

### NtdylibUnloadDll
Unloads a loaded module. Calls `DllMain` with `DLL_PROCESS_DETACH`
and releases the module's virtual memory.

### NtdylibGetModuleHandle
Finds a loaded module by name. Returns the module handle if loaded.

## Supported PE Features

- **Image loading**: Allocates virtual memory at the DLL's preferred base
  or a rebased address
- **Relocations**: `IMAGE_REL_BASED_HIGHLOW` and `IMAGE_REL_BASED_DIR64`
  relocation types
- **Import resolution**: Resolves `IMAGE_IMPORT_DESCRIPTOR` entries
  against loaded modules or ntdll.dll
- **Export lookup**: Parses `IMAGE_EXPORT_DIRECTORY` for name and ordinal
  based lookup
- **Section mapping**: Copies section data with proper page protection

## License

BSD 3-Clause License. See the top-level `LICENSE` file for details.
