/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT NTDYLIB
 * FILE:            ntdylib/src/exports.c
 * PURPOSE:         Export table parsing and import resolution
 * PROGRAMMER:      FreeNT Team
 */

#include "ntdylib.h"

/* Internal helper from loader.c */
extern WCHAR *NtdylibAnsiToUnicode(PCSTR ansi_str);

/*
 * NtdylibFindExport - Find an exported function by name or ordinal.
 *
 * Searches the module's export directory for the requested function.
 * If Name is NULL, the Ordinal is used. Otherwise, the name is searched
 * first, falling back to ordinal if the export is name-only-by-ordinal.
 */
PVOID NtdylibFindExport(PNTDYLIB_MODULE Module, PCSTR Name, WORD Ordinal)
{
    PIMAGE_NT_HEADERS64 headers;
    PIMAGE_DATA_DIRECTORY export_dir_entry;
    PIMAGE_EXPORT_DIRECTORY export_dir;
    PVOID  image_base;
    PDWORD address_of_functions;
    PDWORD address_of_names;
    PWORD  address_of_ordinals;
    ULONG  ordinal_base;
    ULONG  num_functions;
    ULONG  num_names;
    ULONG  i;

    if (Module == NULL || Module->BaseAddress == NULL)
        return NULL;

    image_base = Module->BaseAddress;
    headers = Module->Headers;
    if (headers == NULL)
        return NULL;

    export_dir_entry = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (export_dir_entry->VirtualAddress == 0 ||
        export_dir_entry->Size == 0)
        return NULL;

    export_dir = (PIMAGE_EXPORT_DIRECTORY)
        ((UCHAR *)image_base + export_dir_entry->VirtualAddress);

    num_functions = export_dir->NumberOfFunctions;
    num_names = export_dir->NumberOfNames;
    ordinal_base = export_dir->Base;

    address_of_functions = (PDWORD)
        ((UCHAR *)image_base + export_dir->AddressOfFunctions);
    address_of_names = (PDWORD)
        ((UCHAR *)image_base + export_dir->AddressOfNames);
    address_of_ordinals = (PWORD)
        ((UCHAR *)image_base + export_dir->AddressOfNameOrdinals);

    /* If a name was provided, try to find it by name */
    if (Name != NULL && num_names > 0) {
        for (i = 0; i < num_names; i++) {
            PCSTR export_name = (PCSTR)
                ((UCHAR *)image_base + address_of_names[i]);

            if (freent_strcmp(export_name, Name) == 0) {
                WORD ordinal = address_of_ordinals[i];
                if (ordinal < num_functions) {
                    DWORD rva = address_of_functions[ordinal];
                    if (rva == 0)
                        return NULL; /* Forwarded export */
                    return (UCHAR *)image_base + rva;
                }
            }
        }
    }

    /* Try by ordinal */
    if (Ordinal != 0xFFFF) {
        ULONG func_index = Ordinal - (WORD)ordinal_base;
        if (func_index < num_functions) {
            DWORD rva = address_of_functions[func_index];
            if (rva == 0)
                return NULL;
            return (UCHAR *)image_base + rva;
        }
    }

    return NULL;
}

/*
 * =================================================================
 *  Import Resolution
 * =================================================================
 */

/* Forward declaration for module lookup */
extern PNTDYLIB_MODULE g_LoadedModules;

/*
 * Find a loaded module by its base name.
 * Searches through the loaded module list.
 */
static PNTDYLIB_MODULE NtdylibFindModuleByName(PCWSTR name)
{
    PNTDYLIB_MODULE module;

    if (name == NULL)
        return NULL;

    /* Get the lowercase base name to search for */
    WCHAR search_name[MAX_PATH];
    ULONG len = 0;
    while (name[len] != L'\0' && len < MAX_PATH - 1) {
        search_name[len] = name[len];
        if (search_name[len] >= L'A' && search_name[len] <= L'Z')
            search_name[len] += 32;
        len++;
    }
    search_name[len] = L'\0';

    module = g_LoadedModules;
    while (module != NULL) {
        if (module->BaseName.Buffer != NULL) {
            if (freent_wcscmp(module->BaseName.Buffer, search_name) == 0)
                return module;
        }
        module = module->Next;
    }
    return NULL;
}

/*
 * NtdylibResolveImports - Resolve all imports for a loaded module.
 *
 * Parses the import descriptor table and resolves each imported function
 * from the modules providing them. Uses NtdylibFindExport for lookup.
 */
NTSTATUS NtdylibResolveImports(PNTDYLIB_MODULE Module)
{
    PIMAGE_NT_HEADERS64 headers;
    PIMAGE_DATA_DIRECTORY import_dir;
    PIMAGE_IMPORT_DESCRIPTOR import_desc;
    PVOID image_base;
    ULONG i;

    if (Module == NULL || Module->BaseAddress == NULL)
        return STATUS_INVALID_PARAMETER;

    headers = Module->Headers;
    if (headers == NULL)
        return STATUS_INVALID_PARAMETER;

    image_base = Module->BaseAddress;

    import_dir = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (import_dir->VirtualAddress == 0 || import_dir->Size == 0)
        return STATUS_SUCCESS; /* No imports */

    import_desc = (PIMAGE_IMPORT_DESCRIPTOR)
        ((UCHAR *)image_base + import_dir->VirtualAddress);

    /* Process each import descriptor entry */
    for (i = 0; import_desc[i].Name != 0; i++) {
        PCSTR import_name = (PCSTR)
            ((UCHAR *)image_base + import_desc[i].Name);

        /* Find the provider module among loaded modules */
        PNTDYLIB_MODULE provider = NULL;
        WCHAR *provider_name = NULL;

        if (import_name != NULL && *import_name != '\0') {
            provider_name = NtdylibAnsiToUnicode(import_name);
            if (provider_name != NULL)
                provider = NtdylibFindModuleByName(provider_name);
        }

        /* If not found as ntdylib module, try to find via already-loaded DLLs */
        if (provider == NULL && provider_name != NULL) {
            /* For ntdll.dll and other system DLLs that are already loaded,
               we resolve via GetModuleHandleA + manual export parsing */
            HMODULE hmodule = GetModuleHandleA(import_name);
            if (hmodule != NULL) {
                /* Create a temporary module entry to use the export table parser */
                provider = (PNTDYLIB_MODULE)RtlAllocateHeap(
                    GetProcessHeap(), 0, sizeof(NTDYLIB_MODULE));
                if (provider != NULL) {
                    RtlZeroMemory(provider, sizeof(NTDYLIB_MODULE));
                    provider->BaseAddress = hmodule;
                    provider->Headers = (PIMAGE_NT_HEADERS64)
                        ((UCHAR *)hmodule + ((PIMAGE_DOS_HEADER)hmodule)->e_lfanew);
                    provider->Next = NULL;
                }
            }
        }

        /* Resolve IAT entries */
        if (import_desc[i].FirstThunk != 0 &&
            import_desc[i].u.OriginalFirstThunk != 0) {

            PIMAGE_THUNK_DATA orig_thunk = (PIMAGE_THUNK_DATA)
                ((UCHAR *)image_base + import_desc[i].u.OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)
                ((UCHAR *)image_base + import_desc[i].FirstThunk);

            for (; orig_thunk->u1.Ordinal != 0; orig_thunk++, thunk++) {
                FARPROC proc_addr = NULL;

                if (IMAGE_SNAP_BY_ORDINAL64(orig_thunk->u1.Ordinal)) {
                    /* Import by ordinal */
                    WORD ordinal = IMAGE_ORDINAL16(orig_thunk->u1.Ordinal);
                    if (provider != NULL)
                        proc_addr = NtdylibFindExport(provider, NULL, ordinal);
                } else {
                    /* Import by name */
                    PCSTR func_name = (PCSTR)
                        ((UCHAR *)image_base + orig_thunk->u1.AddressOfData + 2);

                    if (provider != NULL)
                        proc_addr = NtdylibFindExport(provider, func_name, 0xFFFF);
                }

                if (proc_addr != NULL) {
                    /* Patch the IAT */
                    RtlCopyMemory(&thunk->u1.Function, &proc_addr, sizeof(FARPROC));
                }
                /* If not found, leave the IAT entry as 0 */
            }
        }

        /* Free temporary provider if it was created for a system DLL */
        if (provider != NULL && provider->BaseAddress != Module->BaseAddress) {
            /* Check if this is a temporary provider (not in the loaded modules list) */
            BOOLEAN is_temp = TRUE;
            PNTDYLIB_MODULE check = g_LoadedModules;
            while (check != NULL) {
                if (check == provider) {
                    is_temp = FALSE;
                    break;
                }
                check = check->Next;
            }
            if (is_temp)
                RtlFreeHeap(GetProcessHeap(), 0, provider);
        }

        if (provider_name != NULL)
            RtlFreeHeap(GetProcessHeap(), 0, provider_name);
    }

    return STATUS_SUCCESS;
}

/*
 * =================================================================
 *  Public API Functions
 * =================================================================
 */

/*
 * NtdylibGetProcAddress - Resolve an exported function by name or ordinal.
 * Returns a pointer to the function, or NULL if not found.
 */
FARPROC NtdylibGetProcAddress(PNTDYLIB_MODULE Module, LPCSTR lpProcName)
{
    if (Module == NULL)
        return NULL;

    /* Check if lpProcName is an ordinal (high word is 0, low word is the ordinal) */
    ULONGLONG ordinal_val = (ULONGLONG_PTR)lpProcName;
    if (ordinal_val >> 16 == 0) {
        /* Import by ordinal */
        return NtdylibFindExport(Module, NULL, (WORD)ordinal_val);
    }

    /* Import by name */
    return NtdylibFindExport(Module, lpProcName, 0xFFFF);
}

/*
 * NtdylibGetModuleHandle - Find a loaded module by name.
 * Returns the module handle if loaded, NULL otherwise.
 */
PNTDYLIB_MODULE NtdylibGetModuleHandle(LPCWSTR lpModuleName)
{
    if (lpModuleName == NULL)
        return g_LoadedModules; /* First loaded module */

    /* Check loaded modules */
    PNTDYLIB_MODULE module = NtdylibFindModuleByName(lpModuleName);
    if (module != NULL)
        return module;

    return NULL;
}

/*
 * NtdylibUnloadDll - Unload a previously loaded DLL.
 */
BOOLEAN NtdylibUnloadDll(PNTDYLIB_MODULE Module)
{
    if (Module == NULL || Module->BaseAddress == NULL)
        return FALSE;

    /* Decrement reference count */
    if (Module->ReferenceCount > 0)
        Module->ReferenceCount--;

    if (Module->ReferenceCount > 0)
        return TRUE;

    /* Call DllMain with DLL_PROCESS_DETACH */
    if (Module->Headers != NULL) {
        PIMAGE_NT_HEADERS64 headers = Module->Headers;
        if (headers->OptionalHeader.AddressOfEntryPoint != 0) {
            typedef BOOLEAN (NTAPI *PDLL_MAIN)(PVOID, DWORD, PVOID);
            PDLL_MAIN dll_main = (PDLL_MAIN)(
                (UCHAR *)Module->BaseAddress +
                headers->OptionalHeader.AddressOfEntryPoint
            );
            dll_main(Module->BaseAddress, DLL_PROCESS_DETACH, NULL);
        }
    }

    /* Remove from module list */
    {
        PNTDYLIB_MODULE *prev = &g_LoadedModules;
        while (*prev != NULL) {
            if (*prev == Module) {
                *prev = Module->Next;
                break;
            }
            prev = &(*prev)->Next;
        }
    }

    /* Free the module's memory (only if we own the allocation) */
    if (!(Module->Flags & NTDLL_LOAD_LIBRARY_AS_DATAFILE)) {
        PVOID base = Module->BaseAddress;
        NtFreeVirtualMemory((HANDLE)(ULONG_PTR)-1, &base, NULL, MEM_RELEASE);
    }

    /* Free strings */
    if (Module->FullPath.Buffer != NULL)
        RtlFreeHeap(GetProcessHeap(), 0, Module->FullPath.Buffer);
    if (Module->BaseName.Buffer != NULL)
        RtlFreeHeap(GetProcessHeap(), 0, Module->BaseName.Buffer);

    /* Free the module structure */
    RtlFreeHeap(GetProcessHeap(), 0, Module);

    return TRUE;
}

/*
 * NtdylibGetModules - Enumerate loaded modules.
 * Returns the number of loaded modules.
 */
ULONG NtdylibGetModules(PNTDYLIB_MODULE *Modules, ULONG MaxCount)
{
    ULONG count = 0;
    PNTDYLIB_MODULE module = g_LoadedModules;

    while (module != NULL && count < MaxCount) {
        Modules[count++] = module;
        module = module->Next;
    }

    return count;
}

/*
 * NtdylibGetModuleInfo - Get information about a loaded module.
 * Fills in basic module info: base address, size, flags.
 */
NTSTATUS NtdylibGetModuleInfo(PNTDYLIB_MODULE Module, PVOID Info, ULONG InfoSize)
{
    if (Module == NULL || Info == NULL)
        return STATUS_INVALID_PARAMETER;

    if (InfoSize == 0)
        return STATUS_BUFFER_TOO_SMALL;

    /* Fill basic info - in a real implementation, this would
       support various info classes via FileInformationClass */
    if (InfoSize >= sizeof(NTDYLIB_MODULE)) {
        NTDYLIB_MODULE *info = (NTDYLIB_MODULE *)Info;
        info->BaseAddress = Module->BaseAddress;
        info->SizeOfImage = Module->SizeOfImage;
        info->Flags = Module->Flags;
        info->ReferenceCount = Module->ReferenceCount;
        info->Headers = Module->Headers;
        /* Copy strings */
        info->FullPath = Module->FullPath;
        info->BaseName = Module->BaseName;
    } else {
        RtlZeroMemory(Info, InfoSize);
    }

    return STATUS_SUCCESS;
}
