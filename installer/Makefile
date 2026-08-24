# FreeNT Installer build - Windows (WinPE) TUI installer using PDCurses
# Requires PDCurses library and MinGW-w64 cross-toolchain
#
# Build on Linux:
#   make -C installer PDCURSES_DIR=/path/to/pdcurses
#   make -C installer INSTALLER_CC=x86_64-w64-mingw32-gcc

CC ?= x86_64-w64-mingw32-gcc
ARCH ?= x64
BUILD_DIR := ../build/$(ARCH)
TARGET := $(BUILD_DIR)/freent_installer.exe

# PDCurses paths (adjust as needed for your PDCurses install)
PDCURSES_DIR ?= /usr/x86_64-w64-mingw32/sys-root/mingw/include
PDCURSES_LIB ?= /usr/x86_64-w64-mingw32/sys-root/mingw/lib/libpdcurses.a
PDCURSES_INCLUDE ?= $(PDCURSES_DIR)

# CFLAGS for the MinGW cross-compiler
CFLAGS := -std=c11 -Wall -Wextra -I $(PDCURSES_DIR) -DPDCurses
# Linker flags: PDCurses + Windows system libraries
LDFLAGS := -lpdcurses -lole32 -loleaut32 -luuid -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lshell32

.PHONY: all clean
all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/installer.obj: src/installer.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/installer.c -o $@

$(TARGET): $(BUILD_DIR)/installer.obj $(BUILD_DIR)
	$(CC) $(CFLAGS) $(BUILD_DIR)/installer.obj -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(BUILD_DIR)/freent_installer.exe $(BUILD_DIR)/installer.obj
