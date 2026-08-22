# FreeNT Documentation

This directory contains documentation for FreeNT - Alternative Userland for Windows NT.

## Structure

```
docs/
├── README.md              # This file
├── architecture.md        # System architecture
├── installation.md        # Installation guide
├── usage.md               # Usage guide
├── configuration.md       # Configuration reference
├── toolchain.md           # Toolchain setup
├── development.md         # Development guide
├── api/                   # API documentation
│   ├── login_manager.md   # Login manager API
│   ├── winget_wrapper.md  # Winget wrapper API
│   └── core.md            # Core API
└── examples/              # Example code
    ├── login_example.py    # Login manager example
    └── winget_example.py   # Winget wrapper example
```

## Building Documentation

To build the documentation:

```bash
# Install documentation dependencies
pip install -r requirements-dev.txt

# Build documentation (method depends on documentation tool)
# For Sphinx:
cd docs
make html
```

## Contributing to Documentation

1. Fork the repository
2. Create a documentation branch
3. Make your changes
4. Submit a Pull Request

## Documentation Tools

FreeNT uses the following tools for documentation:
- **Sphinx**: For HTML documentation
- **Markdown**: For source files
- **CommonMark**: For Markdown parsing

## Style Guide

- Use clear and concise language
- Include code examples where appropriate
- Document all public APIs
- Keep documentation up to date with code changes
