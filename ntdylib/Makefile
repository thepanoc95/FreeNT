# FreeNT NTDYLIB Build Makefile
# NT Dynamic Library Loader - uses FreeDLL services
#
# This DLL provides PE format DLL loading and unloading,
# import resolution, and export lookup. It depends on FreeDLL.dll
# for memory management, string operations, and NT system calls.

CC := x86_64-w64-mingw32-gcc
ARCH ?= x64
BUILD_DIR := build/$(ARCH)

# Include paths: FreeDLL include first, then ntdylib's own include
INCLUDES := -I include -I ../freedll/include
CFLAGS := -std=gnu11 -ffreestanding -fno-builtin \
          -fno-stack-protector -fno-stack-check -fno-asynchronous-unwind-tables \
          -fno-exceptions -fno-ident -fms-extensions -Wno-discarded-qualifiers \
          -D_WIN64 \
          $(INCLUDES)

# Objects for NTDYLIB
NTDYLIB_OBJECTS := \
	$(BUILD_DIR)/loader.obj \
	$(BUILD_DIR)/exports.obj \
	$(BUILD_DIR)/dllmain.obj

# Default target
all: $(BUILD_DIR)/ntdylib.dll

# Create build directory
$(BUILD_DIR):
	mkdir -p $@

# Compile and link NTDYLIB
$(BUILD_DIR)/ntdylib.dll: $(NTDYLIB_OBJECTS) $(BUILD_DIR)
	$(CC) $(CFLAGS) -shared -nostdlib -o $@ $(NTDYLIB_OBJECTS) -lntdll -L../freedll/build/$(ARCH) -lfreedll -Wl,--out-implib,$(BUILD_DIR)/libntdylib.a -Wl,--entry,NtdylibDllMain

# Object compilation rules
$(BUILD_DIR)/loader.obj: src/loader.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/exports.obj: src/exports.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dllmain.obj: src/dllmain.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Verify that NTDYLIB only imports from ntdll.dll and freedll.dll
check: $(BUILD_DIR)/ntdylib.dll
	@x86_64-w64-mingw32-objdump -p $(BUILD_DIR)/ntdylib.dll | grep -E 'DLL Name:.*ntdll' >/dev/null
	@echo "NTDYLIB correctly imports from ntdll.dll"
	@x86_64-w64-mingw32-objdump -p $(BUILD_DIR)/ntdylib.dll | grep -E 'DLL Name:.*freedll' >/dev/null
	@echo "NTDYLIB correctly imports from freedll.dll"

clean:
	rm -rf build

.PHONY: all clean check
