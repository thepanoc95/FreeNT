# FreeNT BSD TTY Subsystem Build Configuration
# Builds tty.dll using Windows DDK or NTNDK

# Configuration
!ifndef DDK_PATH
DDK_PATH = C:\DDK
!endif

!ifndef FREEDLL_PATH
FREEDLL_PATH = ..\..\freedll
!endif

!ifndef NTNDK_PATH
NTNDK_PATH = C:\NTNDK
!endif

# Compiler and tools
CC = cl.exe
LINK = link.exe
LIB = lib.exe
RC = rc.exe

# Architecture (x86, x64, arm, arm64)
ARCH = x86

# Compilation flags
CFLAGS = /c /W3 /WX /O2 \
         /I"$(DDK_PATH)\inc" \
         /I"$(NTNDK_PATH)\inc" \
         /I"$(FREEDLL_PATH)\inc" \
         /D_WIN32_WINNT=0x0400 \
         /DWIN32 \
         /D_WINDOWS \
         /DUNICODE \
         /D_UNICODE

# Link flags
LDFLAGS = /dll \
          /entry:DllEntryPoint \
          /subsystem:native \
          /map:tty.map \
          /out:tty.dll \
          /implib:tty.lib \
          /def:tty.def

# Library paths
LIBPATH = /libpath:"$(DDK_PATH)\lib\$(ARCH)" \
          /libpath:"$(NTNDK_PATH)\lib\$(ARCH)" \
          /libpath:"$(FREEDLL_PATH)\lib"

# Libraries to link
LIBS = ntdll.lib freedll.lib

# Output directory
OUTDIR = build\$(ARCH)

# Source files
SRCS = tty.c
OBJS = $(OUTDIR)\tty.obj
EXAMPLE_OBJS = $(OUTDIR)\tty_example.obj

# Targets
all: $(OUTDIR)\tty.dll

$(OUTDIR):
	@if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

$(OUTDIR)\tty.obj: tty.c tty.h | $(OUTDIR)
	$(CC) $(CFLAGS) /Fo$@ tty.c

$(OUTDIR)\tty_example.obj: tty_example.c tty.h | $(OUTDIR)
	$(CC) $(CFLAGS) /Fo$@ tty_example.c

$(OUTDIR)\tty.dll: $(OBJS) tty.def | $(OUTDIR)
	$(LINK) $(LDFLAGS) $(LIBPATH) $(LIBS) /out:$@ $(OBJS)

# Build example application
example: $(OUTDIR)\tty_example.obj $(OUTDIR)\tty.lib | $(OUTDIR)
	$(LINK) /subsystem:native /entry:NativeMainEntry \
		$(LIBPATH) $(LIBS) tty.lib \
		/out:$(OUTDIR)\tty_example.exe \
		$(EXAMPLE_OBJS)

# Installation
install: $(OUTDIR)\tty.dll
	@echo Installing TTY subsystem...
	@copy $(OUTDIR)\tty.dll %SystemRoot%\System32\
	@copy $(OUTDIR)\tty.lib %SystemRoot%\System32\

uninstall:
	@echo Uninstalling TTY subsystem...
	@del %SystemRoot%\System32\tty.dll
	@del %SystemRoot%\System32\tty.lib

# Cleanup
clean:
	@echo Cleaning build artifacts...
	@if exist "$(OUTDIR)" rmdir /s /q "$(OUTDIR)"
	@if exist "*.obj" del *.obj
	@if exist "*.map" del *.map
	@if exist "*.exp" del *.exp
	@if exist "*.ilk" del *.ilk
	@if exist "*.pdb" del *.pdb

rebuild: clean all

# Utility targets
info:
	@echo FreeNT BSD TTY Subsystem Build Info
	@echo ====================================
	@echo DDK Path: $(DDK_PATH)
	@echo NTNDK Path: $(NTNDK_PATH)
	@echo FreeNT DLL Path: $(FREEDLL_PATH)
	@echo Architecture: $(ARCH)
	@echo Output Directory: $(OUTDIR)

test: $(OUTDIR)\tty.dll
	@echo Running tests...
	@rem Add test harness here

# Phony targets
.PHONY: all install uninstall clean rebuild info test example

# Help target
help:
	@echo FreeNT BSD TTY Subsystem - Build Targets
	@echo ==========================================
	@echo.
	@echo Targets:
	@echo   all       - Build tty.dll (default)
	@echo   example   - Build example application
	@echo   install   - Install to System32
	@echo   uninstall - Remove from System32
	@echo   clean     - Remove build artifacts
	@echo   rebuild   - Clean and rebuild
	@echo   info      - Show build configuration
	@echo   test      - Run test suite
	@echo   help      - Show this help message
	@echo.
	@echo Variables:
	@echo   DDK_PATH    - Windows DDK installation path
	@echo   NTNDK_PATH  - NTNDK installation path
	@echo   FREEDLL_PATH - FreeNT freedll.dll path
	@echo   ARCH        - Target architecture (x86, x64, arm, arm64)
	@echo.
	@echo Usage:
	@echo   nmake              - Build default (tty.dll)
	@echo   nmake example      - Build example app
	@echo   nmake DDK_PATH=C:\Path\to\DDK all
	@echo   nmake ARCH=x64 all