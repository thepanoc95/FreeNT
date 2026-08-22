# FreeNT Makefile
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# This Makefile provides a build system for FreeNT that works with both
# GNU Make and BSD Make. It supports building, testing, and installing FreeNT.
# It also supports standalone builds for portable deployments.

# Configuration
PROJECT_NAME := FreeNT
VERSION := 0.1.0
PYTHON := python3
PIP := pip3

# Directories
SRC_DIR := src
BUILD_DIR := build
DIST_DIR := dist
DOCS_DIR := docs
SCRIPTS_DIR := scripts
INSTALLERS_DIR := installers
STANDALONE_DIR := standalone

# Files
MAIN_MODULE := $(SRC_DIR)
CONFIG_FILE := $(SRC_DIR)/core/config.py
LOGIN_APP := $(SRC_DIR)/login_manager/login_app.py
WINGET_WRAPPER := $(SRC_DIR)/winget_wrapper/winget.py

# Requirements
REQUIREMENTS_FILE := requirements.txt
REQUIREMENTS_DEV_FILE := requirements-dev.txt

# Python package files
PYTHON_FILES := \
\t$(SRC_DIR)/__init__.py \
\t$(SRC_DIR)/core/__init__.py \
\t$(SRC_DIR)/core/config.py \
\t$(SRC_DIR)/core/utils.py \
\t$(SRC_DIR)/login_manager/__init__.py \
\t$(SRC_DIR)/login_manager/login_app.py \
\t$(SRC_DIR)/winget_wrapper/__init__.py \
\t$(SRC_DIR)/winget_wrapper/winget.py \
\t$(SRC_DIR)/cli.py \
\t$(SRC_DIR)/system/__init__.py \
\t$(SRC_DIR)/system/identity.py \
\t$(SRC_DIR)/system/components.py \
\t$(SRC_DIR)/system/vital.py \
\t$(SRC_DIR)/transform/__init__.py \
\t$(SRC_DIR)/transform/profile.py \
\t$(SRC_DIR)/transform/transformer.py

# Standalone files
STANDALONE_SCRIPTS := \
\t$(STANDALONE_DIR)/freent.bat \
\t$(STANDALONE_DIR)/freent.sh \
\t$(STANDALONE_DIR)/freent-minimal.sh \
\t$(STANDALONE_DIR)/freent-busybox.sh \
\t$(STANDALONE_DIR)/freent-cli.bat \
\t$(STANDALONE_DIR)/freent-cli.sh \
\t$(STANDALONE_DIR)/freent-cli-minimal.sh \
\t$(STANDALONE_DIR)/freent-winget.bat \
\t$(STANDALONE_DIR)/freent-winget.sh \
\t$(STANDALONE_DIR)/freent-winget-minimal.sh \
\t$(STANDALONE_DIR)/start_freent.bat \
\t$(STANDALONE_DIR)/start_freent.sh \
\t$(STANDALONE_DIR)/README.md

# Transform scripts
TRANSFORM_SCRIPTS := \
\t$(SCRIPTS_DIR)/transform/transform.ps1 \
\t$(SCRIPTS_DIR)/transform/rollback.ps1

# Default target
all: build

# Build targets
build: $(BUILD_DIR)/.built

$(BUILD_DIR)/.built: $(PYTHON_FILES)
	@mkdir -p $(BUILD_DIR)
	@echo "Building FreeNT..."
	@touch $@

# Standalone build targets
standalone: $(STANDALONE_DIR)/.standalone_built

$(STANDALONE_DIR)/.standalone_built: $(STANDALONE_SCRIPTS) $(PYTHON_FILES)
	@mkdir -p $(STANDALONE_DIR)/bin
	@mkdir -p $(STANDALONE_DIR)/lib
	@mkdir -p $(STANDALONE_DIR)/etc
	@mkdir -p $(STANDALONE_DIR)/var
	@echo "Building standalone FreeNT..."
	@cp $(STANDALONE_DIR)/freent.bat $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-minimal.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-busybox.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-cli.bat $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-cli.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-cli-minimal.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-winget.bat $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-winget.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@cp $(STANDALONE_DIR)/freent-winget-minimal.sh $(STANDALONE_DIR)/bin/ 2>/dev/null || true
	@touch $@

# Development targets
develop: install-deps install-dev-deps

install-deps:
	@echo "Installing production dependencies..."
	$(PIP) install -r $(REQUIREMENTS_FILE)

install-dev-deps:
	@echo "Installing development dependencies..."
	$(PIP) install -r $(REQUIREMENTS_DEV_FILE)

# Test targets
test: test-unit test-integration

test-unit:
	@echo "Running unit tests..."
	$(PYTHON) -m pytest tests/unit/ -v

test-integration:
	@echo "Running integration tests..."
	$(PYTHON) -m pytest tests/integration/ -v

test-all: test-unit test-integration
	@echo "Running all tests..."
	$(PYTHON) -m pytest tests/ -v

# Clean targets
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)
	rm -rf *.egg-info
	rm -rf __pycache__
	find . -name "*.pyc" -delete
	find . -name "*.pyo" -delete

clean-all: clean
	rm -rf .pytest_cache
	rm -rf .mypy_cache
	rm -rf .coverage

clean-standalone: clean
	rm -rf $(STANDALONE_DIR)/.standalone_built
	rm -rf $(STANDALONE_DIR)/bin
	rm -rf $(STANDALONE_DIR)/lib
	rm -rf $(STANDALONE_DIR)/etc
	rm -rf $(STANDALONE_DIR)/var

# Install targets
install: install-deps
	@echo "Installing FreeNT..."
	$(PIP) install -e .

uninstall:
	@echo "Uninstalling FreeNT..."
	$(PIP) uninstall -y $(PROJECT_NAME)

# Package targets
package: clean
	@echo "Creating distribution packages..."
	$(PYTHON) setup.py sdist bdist_wheel

# Standalone package targets
standalone-package: standalone clean
	@echo "Creating standalone package..."
	@mkdir -p $(DIST_DIR)/standalone
	cp -r $(STANDALONE_DIR)/* $(DIST_DIR)/standalone/ 2>/dev/null || cp -r $(STANDALONE_DIR) $(DIST_DIR)/standalone/ 2>/dev/null
	cp -r $(SRC_DIR) $(DIST_DIR)/standalone/src 2>/dev/null || cp -r $(SRC_DIR) $(DIST_DIR)/standalone/ 2>/dev/null
	cp $(REQUIREMENTS_FILE) $(DIST_DIR)/standalone/ 2>/dev/null || true
	cp $(REQUIREMENTS_DEV_FILE) $(DIST_DIR)/standalone/ 2>/dev/null || true
	cp README.md $(DIST_DIR)/standalone/ 2>/dev/null || true
	cp LICENSE $(DIST_DIR)/standalone/ 2>/dev/null || true
	@echo "Standalone package created in $(DIST_DIR)/standalone"

# Documentation targets
docs: $(DOCS_DIR)/.built

$(DOCS_DIR)/.built:
	@mkdir -p $(DOCS_DIR)
	@echo "Generating documentation..."
	@# TODO: Add documentation generation
	@touch $@

# Utility targets
lint:
	@echo "Running linter..."
	$(PYTHON) -m flake8 $(SRC_DIR) scripts/ tests/ standalone/

format:
	@echo "Formatting code..."
	$(PYTHON) -m black $(SRC_DIR) scripts/ tests/ standalone/
	$(PYTHON) -m isort $(SRC_DIR) scripts/ tests/ standalone/

check-types:
	@echo "Running type checker..."
	$(PYTHON) -m mypy $(SRC_DIR)

# Run targets
run-login-manager:
	@echo "Running login manager..."
	$(PYTHON) -m $(SRC_DIR).login_manager.login_app

run-winget-test:
	@echo "Running winget wrapper test..."
	$(PYTHON) -m $(SRC_DIR).winget_wrapper.winget

run-cli:
	@echo "Running CLI..."
	$(PYTHON) -m $(SRC_DIR).cli

# Standalone run targets
run-standalone:
	@echo "Running standalone FreeNT..."
	@if [ -f $(STANDALONE_DIR)/freent.sh ]; then \
		$(STANDALONE_DIR)/freent.sh; \
	elif [ -f $(STANDALONE_DIR)/freent.bat ]; then \
		$(STANDALONE_DIR)/freent.bat; \
	else \
		echo "No standalone script found"; \
		$(PYTHON) -m $(SRC_DIR).login_manager.login_app; \
	fi

# Windows-specific standalone target
run-standalone-windows:
	@echo "Running standalone FreeNT on Windows..."
	$(STANDALONE_DIR)/freent.bat

# Unix-specific standalone target
run-standalone-unix:
	@echo "Running standalone FreeNT on Unix..."
	$(STANDALONE_DIR)/freent.sh

# Minimal shell target (for BusyBox, etc.)
run-standalone-minimal:
	@echo "Running standalone FreeNT with minimal shell..."
	@if [ -f $(STANDALONE_DIR)/freent-minimal.sh ]; then \
		$(STANDALONE_DIR)/freent-minimal.sh; \
	else \
		$(STANDALONE_DIR)/freent.sh; \
	fi

# System transformation targets
transform: build
	@echo "Transforming system to FreeNT (standard profile)..."
	powershell -ExecutionPolicy Bypass -File scripts/transform/transform.ps1 -Profile standard

transform-minimal: build
	@echo "Transforming system to FreeNT (minimal profile)..."
	powershell -ExecutionPolicy Bypass -File scripts/transform/transform.ps1 -Profile minimal

transform-full: build
	@echo "Transforming system to FreeNT (full profile)..."
	powershell -ExecutionPolicy Bypass -File scripts/transform/transform.ps1 -Profile full

rollback:
	@echo "Rolling back FreeNT transformation..."
	powershell -ExecutionPolicy Bypass -File scripts/transform/rollback.ps1

# Vital-Utilities targets
install-vital-utilities:
	@echo "Installing Vital-Utilities..."
	$(PYTHON) -m $(SRC_DIR).system.vital

update-vital-utilities:
	@echo "Updating Vital-Utilities..."
	$(PYTHON) -m $(SRC_DIR).system.vital --update

# Version info
version:
	@echo "FreeNT version $(VERSION)"

# Help target
help:
	@echo "FreeNT Build System"
	@echo "==================="
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build everything (default)"
	@echo "  build            - Build the project"
	@echo "  standalone       - Build standalone version"
	@echo "  develop          - Install development dependencies"
	@echo "  install-deps     - Install production dependencies"
	@echo "  install-dev-deps - Install development dependencies"
	@echo "  test             - Run all tests"
	@echo "  test-unit        - Run unit tests"
	@echo "  test-integration - Run integration tests"
	@echo "  clean            - Clean build artifacts"
	@echo "  clean-all        - Clean all artifacts"
	@echo "  clean-standalone - Clean standalone build artifacts"
	@echo "  install          - Install FreeNT"
	@echo "  uninstall        - Uninstall FreeNT"
	@echo "  package          - Create distribution packages"
	@echo "  standalone-package - Create standalone package"
	@echo "  docs             - Generate documentation"
	@echo "  lint             - Run linter"
	@echo "  format           - Format code"
	@echo "  check-types      - Run type checker"
	@echo "  run-login-manager - Run the login manager"
	@echo "  run-winget-test  - Run winget wrapper test"
	@echo "  run-cli          - Run the CLI"
	@echo "  run-standalone   - Run standalone FreeNT"
	@echo "  version          - Show version information"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "System Transformation:"
	@echo "  transform        - Transform to FreeNT (standard)"
	@echo "  transform-minimal - Transform to FreeNT (minimal)"
	@echo "  transform-full   - Transform to FreeNT (full)"
	@echo "  rollback         - Rollback transformation"
	@echo "  install-vital-utilities - Install Vital-Utilities"
	@echo "  update-vital-utilities - Update Vital-Utilities"

.PHONY: all build standalone develop install-deps install-dev-deps test test-unit test-integration test-all clean clean-all clean-standalone install uninstall package standalone-package docs lint format check-types run-login-manager run-winget-test run-cli run-standalone run-standalone-windows run-standalone-unix run-standalone-minimal version help transform transform-minimal transform-full rollback install-vital-utilities update-vital-utilities
