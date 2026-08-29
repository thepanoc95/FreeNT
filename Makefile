# FreeNT native build.  This deliberately has no Python, CRT, or Win32 layer.
# Built with MinGW-w64 cross-compiler on Linux targeting Windows x64.
CC := x86_64-w64-mingw32-gcc
ARCH ?= x64
BUILD_DIR := build/$(ARCH)
TARGET := $(BUILD_DIR)/freent.exe

# Freestanding flags: no CRT, no builtins, link only against ntdll
CFLAGS := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -fno-exceptions -fno-ident -Wall -Wextra -Werror -D_WIN64 -D_WIN32_WINNT=0x0600 -DUNICODE -D_UNICODE -I native/include -I ndk
LDFLAGS := -nostdlib -Wl,--entry,freent_entry -Wl,--subsystem,console -lntdll
OBJECTS := $(BUILD_DIR)/entry.obj $(BUILD_DIR)/console.obj $(BUILD_DIR)/command.obj $(BUILD_DIR)/ndk_contract.obj

.PHONY: all clean check-native freedll ntdylib installer
all: $(TARGET) $(BUILD_DIR)/freedll.dll $(BUILD_DIR)/ntdylib.dll

freedll: $(BUILD_DIR)/freedll.dll
ntdylib: $(BUILD_DIR)/ntdylib.dll

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/entry.obj: native/src/entry.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c native/src/entry.c -o $@

$(BUILD_DIR)/console.obj: native/src/console.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c native/src/console.c -o $@

$(BUILD_DIR)/command.obj: native/src/command.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c native/src/command.c -o $@

$(BUILD_DIR)/ndk_contract.obj: native/src/ndk_contract.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c native/src/ndk_contract.c -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# FreeDLL build (companion DLL with Tiny C Runtime)
FREEDLL_DIR := freedll
FREEDLL_CFLAGS := -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -fno-exceptions -fno-ident -fms-extensions -Wno-discarded-qualifiers -Wall -Wextra -D_WIN64 -D_WIN32_WINNT=0x0600 -DUNICODE -D_UNICODE -DVFT_DLL=0x00000002L -DVFT2_UNKNOWN=0x00000000L -I $(FREEDLL_DIR)/include -I $(FREEDLL_DIR)/include/$(ARCH)
FREEDLL_DEFFILE := $(FREEDLL_DIR)/freedll.def
FREEDLL_OBJECTS := $(BUILD_DIR)/crt_memory.obj $(BUILD_DIR)/crt_string.obj $(BUILD_DIR)/crt_format.obj $(BUILD_DIR)/crt_compat.obj $(BUILD_DIR)/crt_stdlib.obj $(BUILD_DIR)/crt_globals.obj $(BUILD_DIR)/heap.obj $(BUILD_DIR)/process.obj $(BUILD_DIR)/stringconv.obj $(BUILD_DIR)/rtl.obj $(BUILD_DIR)/dllmain.obj $(BUILD_DIR)/vmem.obj

$(BUILD_DIR)/crt_memory.obj: $(FREEDLL_DIR)/src/crt_memory.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/crt_memory.c -o $@

$(BUILD_DIR)/crt_string.obj: $(FREEDLL_DIR)/src/crt_string.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/crt_string.c -o $@

$(BUILD_DIR)/crt_format.obj: $(FREEDLL_DIR)/src/crt_format.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/crt_format.c -o $@

$(BUILD_DIR)/crt_compat.obj: $(FREEDLL_DIR)/src/crt_compat.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/crt_compat.c -o $@

$(BUILD_DIR)/crt_stdlib.obj: $(FREEDLL_DIR)/src/crt_stdlib.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/crt_stdlib.c -o $@

$(BUILD_DIR)/crt_globals.obj: $(FREEDLL_DIR)/src/crt_globals.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/crt_globals.c -o $@

$(BUILD_DIR)/heap.obj: $(FREEDLL_DIR)/src/heap.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/heap.c -o $@

$(BUILD_DIR)/process.obj: $(FREEDLL_DIR)/src/process.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/process.c -o $@

$(BUILD_DIR)/stringconv.obj: $(FREEDLL_DIR)/src/stringconv.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/stringconv.c -o $@

$(BUILD_DIR)/rtl.obj: $(FREEDLL_DIR)/src/rtl.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/rtl.c -o $@

$(BUILD_DIR)/dllmain.obj: $(FREEDLL_DIR)/src/dllmain.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/dllmain.c -o $@

$(BUILD_DIR)/vmem.obj: $(FREEDLL_DIR)/src/vmem.c $(BUILD_DIR)
	$(CC) $(FREEDLL_CFLAGS) -c $(FREEDLL_DIR)/src/vmem.c -o $@

$(BUILD_DIR)/freedll.dll: $(FREEDLL_OBJECTS)
	$(CC) -shared -nostdlib -o $@ $(FREEDLL_OBJECTS) -lntdll -Wl,--out-implib,$(BUILD_DIR)/libfreedll.a $(FREEDLL_DEFFILE) -Wl,--entry,DllMain

# NTDYLIB build (NT Dynamic Library Loader - depends on FreeDLL)
NTDYLIB_DIR := ntdylib
NTDYLIB_CFLAGS := -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-stack-check -fno-asynchronous-unwind-tables -fno-exceptions -fno-ident -fms-extensions -Wno-discarded-qualifiers -D_WIN64 -I $(NTDYLIB_DIR)/include -I $(FREEDLL_DIR)/include
NTDYLIB_DEFFILE := $(NTDYLIB_DIR)/ntdylib.def
NTDYLIB_OBJECTS := $(BUILD_DIR)/ntdylib_loader.obj $(BUILD_DIR)/ntdylib_exports.obj $(BUILD_DIR)/ntdylib_dllmain.obj

$(BUILD_DIR)/ntdylib_loader.obj: $(NTDYLIB_DIR)/src/loader.c $(BUILD_DIR)
	$(CC) $(NTDYLIB_CFLAGS) -c $(NTDYLIB_DIR)/src/loader.c -o $@

$(BUILD_DIR)/ntdylib_exports.obj: $(NTDYLIB_DIR)/src/exports.c $(BUILD_DIR)
	$(CC) $(NTDYLIB_CFLAGS) -c $(NTDYLIB_DIR)/src/exports.c -o $@

$(BUILD_DIR)/ntdylib_dllmain.obj: $(NTDYLIB_DIR)/src/dllmain.c $(BUILD_DIR)
	$(CC) $(NTDYLIB_CFLAGS) -c $(NTDYLIB_DIR)/src/dllmain.c -o $@

$(BUILD_DIR)/ntdylib.dll: $(NTDYLIB_OBJECTS)
	$(CC) -shared -nostdlib -o $@ $(NTDYLIB_OBJECTS) -lntdll -L$(BUILD_DIR) -lfreedll -Wl,--out-implib,$(BUILD_DIR)/libntdylib.a $(NTDYLIB_DEFFILE) -Wl,--entry,NtdylibDllMain

check-native: $(TARGET)
	@x86_64-w64-mingw32-objdump -p $(TARGET) | grep -E 'DLL Name: ntdll.dll' >/dev/null
	@! x86_64-w64-mingw32-objdump -p $(TARGET) | grep -E 'DLL Name: (kernel32|kernelbase|ucrtbase|msvcrt|vcruntime)[.]dll' >/dev/null

# FreeNT Installer (WinPE TUI installer with PDCurses)
# This is a Windows application that runs inside WinPE
# Requires PDCurses - set PDCURSES_DIR to the PDCurses include directory
INSTALLER_CC ?= x86_64-w64-mingw32-gcc
INSTALLER_CFLAGS := -std=c11 -Wall -Wextra -I $(PDCURSES_DIR)
INSTALLER_LDFLAGS := -L$(BUILD_DIR)/lib -L$(PDCURSES_DIR)/wincon -lpdcurses -lole32 -loleaut32 -luuid -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lshell32

installer: check-pdcurses $(BUILD_DIR)/freent_installer.exe

check-pdcurses:
ifeq ($(PDCURSES_DIR),)
	@echo "PDCURSES_DIR must be set to build the installer."
	@echo "Example: make installer PDCURSES_DIR=/usr/include/pdcurses"
	@exit 1
endif

$(BUILD_DIR)/freent_installer.exe: installer/src/installer.c $(BUILD_DIR)
	$(INSTALLER_CC) $(INSTALLER_CFLAGS) -c installer/src/installer.c -o $(BUILD_DIR)/installer.obj
	$(INSTALLER_CC) $(BUILD_DIR)/installer.obj -o $(BUILD_DIR)/freent_installer.exe $(INSTALLER_LDFLAGS)

clean:
	rm -rf build
