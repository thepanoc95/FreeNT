# Standalone Makefile
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# This Makefile is for building the standalone version of FreeNT
# It can be used separately or as part of the main build system

# Configuration
STANDALONE_DIR := .
SRC_DIR := ..

# Scripts
SCRIPTS := \
\tfreent.bat \
\tfreent.sh \
\tfreent-minimal.sh \
\tfreent-busybox.sh \
\tfreent-cli.bat \
\tfreent-cli.sh \
\tfreent-cli-minimal.sh \
\tfreent-winget.bat \
\tfreent-winget.sh \
\tfreent-winget-minimal.sh \
\tstart_freent.bat \
\tstart_freent.sh

# Default target
all: build

# Build standalone
build: bin/.built

bin/.built: $(SCRIPTS)
	@mkdir -p bin
	@echo "Building standalone FreeNT..."
	cp freent.bat bin/ 2>/dev/null || true
	cp freent.sh bin/ 2>/dev/null || true
	cp freent-minimal.sh bin/ 2>/dev/null || true
	cp freent-busybox.sh bin/ 2>/dev/null || true
	cp freent-cli.bat bin/ 2>/dev/null || true
	cp freent-cli.sh bin/ 2>/dev/null || true
	cp freent-cli-minimal.sh bin/ 2>/dev/null || true
	cp freent-winget.bat bin/ 2>/dev/null || true
	cp freent-winget.sh bin/ 2>/dev/null || true
	cp freent-winget-minimal.sh bin/ 2>/dev/null || true
	@touch bin/.built

# Make scripts executable (Unix)
make-executable:
	@echo "Making scripts executable..."
	chmod +x *.sh 2>/dev/null || true
	chmod +x bin/*.sh 2>/dev/null || true

# Clean
clean:
	@echo "Cleaning standalone build..."
	rm -rf bin/.built
	rm -rf bin

# Install to system (optional)
install:
	@echo "Installing standalone FreeNT..."
	@mkdir -p /usr/local/bin
	cp bin/freent.sh /usr/local/bin/freent 2>/dev/null || true
	cp bin/freent-cli.sh /usr/local/bin/freent-cli 2>/dev/null || true
	cp bin/freent-winget.sh /usr/local/bin/freent-winget 2>/dev/null || true
	chmod +x /usr/local/bin/freent* 2>/dev/null || true

# Uninstall
uninstall:
	@echo "Uninstalling standalone FreeNT..."
	rm -f /usr/local/bin/freent 2>/dev/null || true
	rm -f /usr/local/bin/freent-cli 2>/dev/null || true
	rm -f /usr/local/bin/freent-winget 2>/dev/null || true

# Help
help:
	@echo "Standalone FreeNT Makefile"
	@echo "=========================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build standalone FreeNT"
	@echo "  build        - Build standalone version"
	@echo "  make-executable - Make shell scripts executable"
	@echo "  clean        - Clean build artifacts"
	@echo "  install      - Install to system (Unix)"
	@echo "  uninstall    - Uninstall from system (Unix)"
	@echo "  help         - Show this help"

.PHONY: all build make-executable clean install uninstall help
