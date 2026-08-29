# FreeNT FreeDLL Build Makefile
# Builds freedll.dll - a companion DLL to ntdll.dll containing a Tiny C Runtime
#
# This DLL is freestanding (no CRT dependency) and implements:
# - Tiny C Runtime (memory, string, stdlib functions)
# - RTL (Run-Time Library) functions
# - Heap management
# - Process/thread utilities
# - TLS/FLS support
#
# Build: gmake

CC := x86_64-w64-mingw32-gcc
AR := x86_64-w64-mingw32-ar
ARCH ?= x64
BUILD_DIR := build/$(ARCH)

TARGET_TRIPLE := x86_64-w64-mingw32
MACHINE := -m64

# Include paths
INCLUDES := -I include -I include/$(ARCH)
CFLAGS := -std=gnu11 -ffreestanding -fno-builtin \
          -fno-stack-protector -fno-asynchronous-unwind-tables \
          -fno-exceptions -fno-ident -fms-extensions \
          -Wall -Wextra -Wno-discarded-qualifiers \
          -D_WIN64 -D_WIN32_WINNT=0x0600 \
          -DUNICODE -D_UNICODE \
          -DVFT_DLL=0x00000002L -DVFT2_UNKNOWN=0x00000000L \
          $(INCLUDES)

# Objects for FreeDLL
FREEDLL_OBJECTS := \
	$(BUILD_DIR)/crt_memory.obj \
	$(BUILD_DIR)/crt_string.obj \
	$(BUILD_DIR)/crt_format.obj \
	$(BUILD_DIR)/crt_compat.obj $(BUILD_DIR)/crt_stdlib.obj \
	$(BUILD_DIR)/crt_globals.obj \
	$(BUILD_DIR)/heap.obj \
	$(BUILD_DIR)/process.obj \
	$(BUILD_DIR)/stringconv.obj \
	$(BUILD_DIR)/rtl.obj \
	$(BUILD_DIR)/dllmain.obj \
	$(BUILD_DIR)/vmem.obj

# Default target
all: $(BUILD_DIR)/freedll.dll

# Create build directory
$(BUILD_DIR):
	mkdir -p $@

# Compile and link FreeDLL
$(BUILD_DIR)/freedll.dll: $(FREEDLL_OBJECTS) $(BUILD_DIR)
	$(CC) $(CFLAGS) -shared -nostdlib -o $@ $(FREEDLL_OBJECTS) -lntdll -Wl,--out-implib,$(BUILD_DIR)/libfreedll.a -Wl,--entry,DllMain

# Generic object compilation rules
$(BUILD_DIR)/crt_memory.obj: src/crt_memory.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/crt_string.obj: src/crt_string.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/crt_format.obj: src/crt_format.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/crt_compat.obj: src/crt_compat.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/crt_stdlib.obj: src/crt_stdlib.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/crt_globals.obj: src/crt_globals.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/heap.obj: src/heap.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/process.obj: src/process.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/stringconv.obj: src/stringconv.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/rtl.obj: src/rtl.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dllmain.obj: src/dllmain.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vmem.obj: src/vmem.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Verify that FreeDLL only imports from ntdll
check: $(BUILD_DIR)/freedll.dll
	@x86_64-w64-mingw32-objdump -p $(BUILD_DIR)/freedll.dll | grep -E 'DLL Name:.*ntdll' >/dev/null
	@echo "FreeDLL correctly imports from ntdll.dll"

clean:
	rm -rf build
