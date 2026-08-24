/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT NTDYLIB
 * FILE:            ntdylib/src/loader.c
 * PURPOSE:         NT Dynamic Library Loader - PE parsing and loading
 * PROGRAMMER:      FreeNT Team
 */

#include "ntdylib.h"

/* Module list */
PNTDYLIB_MODULE g_LoadedModules = NULL;
BOOLEAN          g_NtdylibInitialized = FALSE;

/*
 * ============================================================
 *  Internal helpers
 * ============================================================
 */

/*
 * NtdylibInit - Initialize the module loader.
 * Must be called before any other NTDYLIB function.
 */
NTSTATUS NtdylibInit(VOID)
{
    if (g_NtdylibInitialized)
        return STATUS_SUCCESS;

    /* Ensure FreeDLL's heap is available */
    GetProcessHeap();

    g_LoadedModules = NULL;
    g_NtdylibInitialized = TRUE;
    return STATUS_SUCCESS;
}

/*
 * NtdylibCleanup - Clean up all loaded modules.
 * Called during process exit.
 */
VOID NtdylibCleanup(VOID)
{
    PNTDYLIB_MODULE module, next;

    module = g_LoadedModules;
    while (module != NULL) {
        next = module->Next;

        if (module->BaseAddress != NULL)
            NtFreeVirtualMemory(
                (HANDLE)(ULONG_PTR)-1,
                &module->BaseAddress,
                NULL,
                MEM_RELEASE
            );

        RtlFreeHeap(GetProcessHeap(), 0, module);
        module = next;
    }
    g_LoadedModules = NULL;
    g_NtdylibInitialized = FALSE;
}

/*
 * Create a temporary ANSI string from a Unicode string.
 * Returns allocated buffer that must be freed.
 */
static char *NtdylibUnicodeToAnsi(PCWSTR unicode_str, ULONG max_chars)
{
    char *ansi;
    ULONG len = 0;
    ULONG cap;

    if (unicode_str == NULL)
        return NULL;

    while (unicode_str[len] != L'\0' && len < max_chars)
        len++;

    cap = len + 1;
    ansi = (char *)RtlAllocateHeap(GetProcessHeap(), 0, cap);
    if (ansi == NULL)
        return NULL;

    for (ULONG i = 0; i < len; i++)
        ansi[i] = (char)(unicode_str[i] & 0xFF);
    ansi[len] = '\0';
    return ansi;
}

/*
 * Convert ANSI to Unicode.
 */
WCHAR *NtdylibAnsiToUnicode(PCSTR ansi_str)
{
    WCHAR *unicode;
    ULONG len = 0;

    if (ansi_str == NULL)
        return NULL;

    while (ansi_str[len] != '\0')
        len++;

    unicode = (WCHAR *)RtlAllocateHeap(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (unicode == NULL)
        return NULL;

    for (ULONG i = 0; i < len; i++)
        unicode[i] = (WCHAR)(unsigned char)ansi_str[i];
    unicode[len] = L'\0';
    return unicode;
}

/*
 * Get file size using NtQueryInformationFile.
 */
static NTSTATUS NtdylibGetFileSize(HANDLE FileHandle, DWORD *FileSize)
{
    IO_STATUS_BLOCK iosb;
    FILE_STANDARD_INFORMATION info;
    NTSTATUS status;

    status = NtQueryInformationFile(
        FileHandle,
        &iosb,
        &info,
        sizeof(info),
        FileStandardInformation
    );

    if (status == STATUS_SUCCESS)
        *FileSize = (DWORD)info.Size.QuadPart;

    return status;
}

/*
 * Read entire file into memory.
 * Returns allocated buffer; caller must free via RtlFreeHeap.
 */
static PVOID NtdylibReadFile(HANDLE FileHandle, SIZE_T *BytesRead)
{
    IO_STATUS_BLOCK iosb;
    BYTE   buffer[4096];
    PVOID  data = NULL;
    SIZE_T total_size = 0;
    SIZE_T offset = 0;
    NTSTATUS status;

    while (TRUE) {
        status = NtReadFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            buffer,
            sizeof(buffer),
            NULL,
            NULL
        );

        if (!NT_SUCCESS(status)) {
            if (status == STATUS_END_OF_FILE)
                break;
            if (data != NULL)
                RtlFreeHeap(GetProcessHeap(), 0, data);
            return NULL;
        }

        if (iosb.Information == 0)
            break;

        /* Grow the buffer */
        {
            PVOID new_data = RtlReAllocateHeap(
                GetProcessHeap(), 0, data, total_size + iosb.Information + 1);
            if (new_data == NULL) {
                if (data != NULL)
                    RtlFreeHeap(GetProcessHeap(), 0, data);
                return NULL;
            }
            data = new_data;
        }

        RtlCopyMemory((UCHAR *)data + offset, buffer, iosb.Information);
        offset += iosb.Information;
        total_size = offset;
    }

    *BytesRead = total_size;
    return data;
}

/*
 * =================================================================
 *  PE Parsing and Loading
 * =================================================================
 */

/*
 * NtdylibParseHeaders - Parse PE headers of a loaded image.
 * Given an image base, extracts and validates the PE headers.
 */
NTSTATUS NtdylibParseHeaders(PVOID ImageBase, PIMAGE_NT_HEADERS64 *Headers)
{
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS64 nt_headers;

    if (ImageBase == NULL || Headers == NULL)
        return STATUS_INVALID_PARAMETER;

    *Headers = NULL;

    /* Verify DOS signature */
    dos_header = (PIMAGE_DOS_HEADER)ImageBase;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_PARAMETER;

    /* Get NT headers */
    nt_headers = (PIMAGE_NT_HEADERS64)
        ((UCHAR *)ImageBase + dos_header->e_lfanew);

    /* Verify PE signature */
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_PARAMETER;

    *Headers = nt_headers;
    return STATUS_SUCCESS;
}

/*
 * NtdylibParseHeaders32 - Parse PE headers of a 32-bit loaded image.
 */
NTSTATUS NtdylibParseHeaders32(PVOID ImageBase, PIMAGE_NT_HEADERS32 *Headers)
{
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS32 nt_headers;

    if (ImageBase == NULL || Headers == NULL)
        return STATUS_INVALID_PARAMETER;

    *Headers = NULL;

    dos_header = (PIMAGE_DOS_HEADER)ImageBase;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_PARAMETER;

    nt_headers = (PIMAGE_NT_HEADERS32)
        ((UCHAR *)ImageBase + dos_header->e_lfanew);

    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_PARAMETER;

    *Headers = nt_headers;
    return STATUS_SUCCESS;
}

/*
 * NtdylibMapImageSections - Map PE sections into memory at the image base.
 * Called after memory has been allocated for the image.
 */
static NTSTATUS NtdylibMapImageSections(
    PVOID ImageBase, PVOID LoadBase,
    PIMAGE_NT_HEADERS64 Headers)
{
    ULONG i;
    PIMAGE_SECTION_HEADER section;
    DWORD section_size;
    DWORD page_size = 4096;

    /* Calculate the section header table position */
    section = (PIMAGE_SECTION_HEADER)
        ((UCHAR *)&Headers->OptionalHeader + Headers->FileHeader.SizeOfOptionalHeader);

    for (i = 0; i < Headers->FileHeader.NumberOfSections; i++) {
        DWORD dest_rva = section[i].VirtualAddress;
        DWORD src_offset = section[i].PointerToRawData;
        DWORD copy_size = section[i].SizeOfRawData;
        DWORD virt_size = section[i].Misc.VirtualSize;

        if (copy_size == 0) {
            /* Section has no raw data, just virtual size */
            if (virt_size == 0)
                continue;
            copy_size = virt_size;
        }

        if (virt_size == 0)
            virt_size = copy_size;

        /* Determine protection flags */
        DWORD protect = PAGE_NOACCESS;
        DWORD characteristics = section[i].Characteristics;

        if (characteristics & 0x40000000) {             /* IMAGE_SCN_MEM_EXECUTE */
            if (characteristics & 0x80000000) {          /* IMAGE_SCN_MEM_WRITE */
                protect = PAGE_EXECUTE_READWRITE;
            } else if (characteristics & 0x20000000) {   /* IMAGE_SCN_MEM_READ */
                protect = PAGE_EXECUTE_READ;
            } else {
                protect = PAGE_EXECUTE_READ;
            }
        } else if (characteristics & 0x80000000) {
            protect = PAGE_READWRITE;
        } else {
            protect = PAGE_READONLY;
        }

        /* Copy section data */
        PVOID dest = (UCHAR *)LoadBase + dest_rva;
        if (src_offset > 0 && copy_size > 0) {
            RtlCopyMemory(dest, (UCHAR *)ImageBase + src_offset, copy_size);
        }

        /* Zero-fill the rest of the virtual size */
        if (virt_size > copy_size) {
            RtlZeroMemory(
                (UCHAR *)dest + copy_size,
                virt_size - copy_size
            );
        }

        /* Apply page protection */
        SIZE_T region_size = virt_size;
        PVOID region_base = dest;
        ULONG old_protect;
        NtProtectVirtualMemory(
            (HANDLE)(ULONG_PTR)-1,
            &region_base,
            &region_size,
            protect,
            &old_protect
        );
    }

    return STATUS_SUCCESS;
}

/*
 * NtdylibProcessRelocs - Apply base relocations to a loaded module.
 * Called when the image is loaded at an address different from its preferred base.
 */
NTSTATUS NtdylibProcessRelocs(PNTDYLIB_MODULE Module, PVOID LoadBase)
{
    ULONG_PTR delta;
    PIMAGE_NT_HEADERS64 headers;
    PIMAGE_DATA_DIRECTORY reloc_dir;
    PIMAGE_BASE_RELOCATION reloc;
    PBYTE reloc_end;
    DWORD page_size;

    if (Module == NULL || LoadBase == NULL)
        return STATUS_INVALID_PARAMETER;

    headers = Module->Headers;
    if (headers == NULL)
        return STATUS_INVALID_PARAMETER;

    reloc_dir = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (reloc_dir->Size == 0 || reloc_dir->VirtualAddress == 0)
        return STATUS_SUCCESS; /* No relocations needed */

    /* Calculate the relocation delta */
    delta = (ULONG_PTR)LoadBase - headers->OptionalHeader.ImageBase;
    if (delta == 0)
        return STATUS_SUCCESS; /* Loaded at preferred base, no relocs needed */

    reloc = (PIMAGE_BASE_RELOCATION)
        ((UCHAR *)LoadBase + reloc_dir->VirtualAddress);
    reloc_end = (PBYTE)reloc + reloc_dir->Size;

    page_size = 4096;

    /* Process each relocation block */
    while ((PBYTE)reloc < reloc_end && reloc->SizeOfBlock > 0) {
        DWORD page_rva = reloc->VirtualAddress;
        DWORD block_size = reloc->SizeOfBlock;
        DWORD num_entries = (block_size - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        PWORD type_offsets = (PWORD)((PBYTE)reloc + sizeof(IMAGE_BASE_RELOCATION));

        PVOID page_base = (UCHAR *)LoadBase + page_rva;

        for (DWORD i = 0; i < num_entries; i++) {
            WORD entry = type_offsets[i];
            WORD type = (entry >> 12) & 0xF;
            WORD offset = entry & 0xFFF;

            if (type == 0)
                continue; /* IMAGE_REL_BASED_ABSOLUTE */

            PVOID *patch_addr = (PVOID *)((UCHAR *)page_base + offset);

            if (type == IMAGE_REL_BASED_HIGHLOW) {
                /* 32-bit relocation */
                DWORD *ptr32 = (DWORD *)patch_addr;
                *ptr32 = (DWORD)(*ptr32 + delta);
            }
            else if (type == IMAGE_REL_BASED_DIR64) {
                /* 64-bit relocation */
                ULONGLONG *ptr64 = (ULONGLONG *)patch_addr;
                *ptr64 = *ptr64 + (ULONGLONG)delta;
            }
            /* Other relocation types are not supported */
        }

        reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)reloc + block_size);
    }

    return STATUS_SUCCESS;
}

/*
 * =================================================================
 *  Public API: NtdylibLoadDll
 * =================================================================
 */

/*
 * NtdylibLoadDll - Load a DLL from a file path.
 *
 * This implements a minimal PE loader:
 * 1. Reads the DLL file into memory
 * 2. Allocates virtual memory at the image base
 * 3. Copies section headers and data
 * 4. Applies relocations if needed
 * 5. Resolves imports
 * 6. Calls the DLL's entry point (DllMain/DllEntryPoint)
 */
PNTDYLIB_MODULE NtdylibLoadDll(LPCWSTR lpLibFileName, DWORD dwFlags)
{
    PNTDYLIB_MODULE       module;
    HANDLE                file_handle = NULL;
    UNICODE_STRING        file_name;
    OBJECT_ATTRIBUTES     obj_attr;
    IO_STATUS_BLOCK       iosb;
    PVOID                 image_data = NULL;
    SIZE_T                image_size = 0;
    PVOID                 load_base = NULL;
    PIMAGE_NT_HEADERS64   headers = NULL;
    PIMAGE_NT_HEADERS64   map_headers = NULL;
    NTSTATUS              status;
    ULONG                 i;
    SIZE_T                alloc_size = 0;

    if (!g_NtdylibInitialized) {
        status = NtdylibInit();
        if (!NT_SUCCESS(status))
            return NULL;
    }

    if (lpLibFileName == NULL || *lpLibFileName == L'\0')
        return NULL;

    /* Check if already loaded */
    module = NtdylibGetModuleHandle(lpLibFileName);
    if (module != NULL) {
        module->ReferenceCount++;
        return module;
    }

    /* Convert filename to ANSI for building the device path */
    {
        char *ansi_name = NtdylibUnicodeToAnsi(lpLibFileName, MAX_PATH);
        if (ansi_name == NULL)
            return NULL;

        /* Build full path with NT device prefix */
        char full_path[MAX_PATH];
        freent_strcpy(full_path, "\\??\\");
        freent_strcat(full_path, ansi_name);

        /* Convert back to Unicode for NtCreateFile */
        WCHAR *unicode_name = NtdylibAnsiToUnicode(full_path);
        if (unicode_name == NULL) {
            RtlFreeHeap(GetProcessHeap(), 0, ansi_name);
            return NULL;
        }

        RtlFreeHeap(GetProcessHeap(), 0, ansi_name);

        /* Set up object attributes for file open */
        RtlZeroMemory(&file_name, sizeof(file_name));
        file_name.Buffer = unicode_name;
        file_name.Length = (USHORT)(freent_wcslen(unicode_name) * sizeof(WCHAR));
        file_name.MaximumLength = file_name.Length;

        RtlZeroMemory(&obj_attr, sizeof(obj_attr));
        obj_attr.Length = sizeof(obj_attr);
        obj_attr.Attributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
        obj_attr.ObjectName = &file_name;
        obj_attr.SecurityDescriptor = NULL;
        obj_attr.SecurityQualityOfService = NULL;

        RtlFreeHeap(GetProcessHeap(), 0, unicode_name);

        /* Open the file */
        status = NtCreateFile(
            &file_handle,
            GENERIC_READ,
            &obj_attr,
            &iosb,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE,
            NULL,
            0
        );
    }

    if (!NT_SUCCESS(status) || file_handle == NULL)
        return NULL;

    /* Read file data into memory */
    image_data = NtdylibReadFile(file_handle, &image_size);
    NtClose(file_handle);

    if (image_data == NULL || image_size == 0)
        return NULL;

    /* Parse PE headers from the in-memory copy */
    status = NtdylibParseHeaders(image_data, &headers);
    if (!NT_SUCCESS(status) || headers == NULL) {
        RtlFreeHeap(GetProcessHeap(), 0, image_data);
        return NULL;
    }

    /* Determine the total virtual size needed */
    alloc_size = (SIZE_T)headers->OptionalHeader.SizeOfImage;
    if (alloc_size == 0)
        alloc_size = (SIZE_T)headers->OptionalHeader.SizeOfHeaders;

    /* Try to allocate at the preferred base */
    load_base = (PVOID)headers->OptionalHeader.ImageBase;
    status = NtAllocateVirtualMemory(
        (HANDLE)(ULONG_PTR)-1,
        &load_base,
        0,
        &alloc_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!NT_SUCCESS(status) || load_base == NULL) {
        /* Preferred base not available, let the system choose */
        load_base = NULL;
        alloc_size = (SIZE_T)headers->OptionalHeader.SizeOfImage;
        if (alloc_size == 0)
            alloc_size = (SIZE_T)headers->OptionalHeader.SizeOfHeaders;
        status = NtAllocateVirtualMemory(
            (HANDLE)(ULONG_PTR)-1,
            &load_base,
            0,
            &alloc_size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );

        if (!NT_SUCCESS(status) || load_base == NULL) {
            RtlFreeHeap(GetProcessHeap(), 0, image_data);
            return NULL;
        }
    }

    /* Zero the allocated memory */
    RtlZeroMemory(load_base, alloc_size);

    /* Copy headers */
    RtlCopyMemory(
        load_base,
        image_data,
        headers->OptionalHeader.SizeOfHeaders
    );

    /* Get the mapped headers in the loaded image space */
    map_headers = (PIMAGE_NT_HEADERS64)
        ((UCHAR *)load_base + ((PIMAGE_DOS_HEADER)image_data)->e_lfanew);

    /* Map sections */
    status = NtdylibMapImageSections(image_data, load_base, headers);

    /* Free the in-memory image copy */
    RtlFreeHeap(GetProcessHeap(), 0, image_data);

    if (!NT_SUCCESS(status)) {
        NtFreeVirtualMemory((HANDLE)(ULONG_PTR)-1, &load_base, NULL, MEM_RELEASE);
        return NULL;
    }

    /* Allocate the module entry structure */
    module = (PNTDYLIB_MODULE)RtlAllocateHeap(GetProcessHeap(), 0, sizeof(NTDYLIB_MODULE));
    if (module == NULL) {
        NtFreeVirtualMemory((HANDLE)(ULONG_PTR)-1, &load_base, NULL, MEM_RELEASE);
        return NULL;
    }

    /* Initialize the module entry */
    RtlZeroMemory(module, sizeof(NTDYLIB_MODULE));
    module->BaseAddress = load_base;
    module->SizeOfImage = (DWORD)alloc_size;
    module->Flags = dwFlags;
    module->ReferenceCount = 1;
    module->Headers = map_headers;

    /* Build the module name (lowercase base name) */
    {
        WCHAR base_name_buffer[MAX_PATH];
        USHORT last_sep = 0;
        USHORT path_len = 0;

        while (lpLibFileName[path_len] != L'\0')
            path_len++;

        for (i = 0; i < path_len; i++) {
            if (lpLibFileName[i] == L'\\' || lpLibFileName[i] == L'/')
                last_sep = i + 1;
        }

        USHORT name_len = path_len - last_sep;
        for (i = 0; i < name_len && i < MAX_PATH - 1; i++)
            base_name_buffer[i] = lpLibFileName[last_sep + i];
        base_name_buffer[name_len] = L'\0';

        for (i = 0; i < name_len; i++) {
            if (base_name_buffer[i] >= L'A' && base_name_buffer[i] <= L'Z')
                base_name_buffer[i] += 32;
        }

        /* Store full path */
        USHORT full_len = path_len;
        module->FullPath.Buffer = (WCHAR *)RtlAllocateHeap(
            GetProcessHeap(), 0, (full_len + 1) * sizeof(WCHAR));
        if (module->FullPath.Buffer != NULL) {
            RtlCopyMemory(module->FullPath.Buffer, lpLibFileName, full_len * sizeof(WCHAR));
            module->FullPath.Buffer[full_len] = L'\0';
            module->FullPath.Length = full_len * sizeof(WCHAR);
            module->FullPath.MaximumLength = (full_len + 1) * sizeof(WCHAR);
        }

        /* Store base name */
        module->BaseName.Buffer = (WCHAR *)RtlAllocateHeap(
            GetProcessHeap(), 0, (name_len + 1) * sizeof(WCHAR));
        if (module->BaseName.Buffer != NULL) {
            RtlCopyMemory(module->BaseName.Buffer, base_name_buffer, name_len * sizeof(WCHAR));
            module->BaseName.Buffer[name_len] = L'\0';
            module->BaseName.Length = name_len * sizeof(WCHAR);
            module->BaseName.MaximumLength = (name_len + 1) * sizeof(WCHAR);
        }
    }

    /* Apply relocations if we didn't load at the preferred base */
    if (load_base != (PVOID)(ULONG_PTR)headers->OptionalHeader.ImageBase) {
        status = NtdylibProcessRelocs(module, load_base);
        if (!NT_SUCCESS(status)) {
            if (module->FullPath.Buffer)
                RtlFreeHeap(GetProcessHeap(), 0, module->FullPath.Buffer);
            if (module->BaseName.Buffer)
                RtlFreeHeap(GetProcessHeap(), 0, module->BaseName.Buffer);
            RtlFreeHeap(GetProcessHeap(), 0, module);
            NtFreeVirtualMemory((HANDLE)(ULONG_PTR)-1, &load_base, NULL, MEM_RELEASE);
            return NULL;
        }
    }

    /* Link into the module list */
    module->Next = g_LoadedModules;
    g_LoadedModules = module;

    /* Resolve imports */
    status = NtdylibResolveImports(module);

    /* Call the DLL entry point if present */
    if (NT_SUCCESS(status) &&
        map_headers->OptionalHeader.AddressOfEntryPoint != 0) {

        PVOID entry_point = (UCHAR *)load_base +
            map_headers->OptionalHeader.AddressOfEntryPoint;

        typedef BOOLEAN (NTAPI *PDLL_MAIN)(PVOID, DWORD, PVOID);
        PDLL_MAIN dll_main = (PDLL_MAIN)entry_point;

        dll_main(load_base, DLL_PROCESS_ATTACH, NULL);
    }

    return module;
}
