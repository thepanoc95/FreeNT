# FreeNT Standalone Mode

FreeNT can run in standalone mode without requiring a system-wide Python installation. This is useful for:
- Portable deployments
- Systems where you can't install Python system-wide
- Testing different Python versions
- Running from USB drives

## Standalone Directory Structure

```
standalone/
├── bin/                    # Executable scripts
│   ├── freent.bat          # Windows batch launcher
│   ├── freent.sh           # Unix shell launcher
│   ├── freent-cli.bat      # CLI launcher (Windows)
│   ├── freent-cli.sh       # CLI launcher (Unix)
│   ├── freent-winget.bat   # Winget wrapper launcher (Windows)
│   └── freent-winget.sh    # Winget wrapper launcher (Unix)
├── lib/                    # Python libraries (optional)
├── etc/                    # Configuration files
├── var/                    # Variable data
├── python/                 # Portable Python (optional)
│   ├── python.exe          # Python executable
│   ├── python              # Python executable (Unix)
│   └── ...                 # Python libraries
├── start_freent.bat        # Windows startup script
├── start_freent.sh         # Unix startup script
└── README.md               # This file
```

## Usage

### Windows

1. **With system Python installed**:
   ```cmd
   standalone\freent.bat
   ```

2. **With portable Python**:
   - Download portable Python from [python.org](https://www.python.org/downloads/windows/)
   - Extract it to `standalone\python`
   - Run:
     ```cmd
     standalone\freent.bat
     ```

3. **Using startup script**:
   ```cmd
   standalone\start_freent.bat
   ```

### Unix/Linux (WSL, Cygwin, etc.)

1. **With system Python installed**:
   ```bash
   chmod +x standalone/freent.sh
   ./standalone/freent.sh
   ```

2. **With portable Python**:
   - Download portable Python
   - Extract it to `standalone/python`
   - Make scripts executable:
     ```bash
     chmod +x standalone/*.sh
     ```
   - Run:
     ```bash
     ./standalone/freent.sh
     ```

3. **Using startup script**:
   ```bash
   ./standalone/start_freent.sh
   ```

## Bash/Shell Compatibility

The shell scripts (`*.sh`) are designed to work with:
- **bash** - GNU Bourne Again Shell
- **sh** - Standard POSIX shell
- **dash** - Debian Almquist shell
- **ash** - Almquist shell
- **zsh** - Z Shell
- **ksh** - Korn Shell
- **busybox sh** - BusyBox shell

### Shell Features Used

The scripts use only POSIX-compatible features:
- `#!/bin/sh` shebang
- `command -v` for checking command existence (POSIX)
- `dirname` for path manipulation (POSIX)
- Standard shell variables and conditionals
- No bash-specific features (arrays, `[[ ]]`, etc.)

### BusyBox Support

For systems using BusyBox (common in embedded systems or minimal environments):

1. Ensure BusyBox has the following applets:
   - `sh` or `ash`
   - `command` or `which`
   - `dirname`
   - `basename`

2. The scripts should work with BusyBox's minimal shell implementation.

3. Test with:
   ```bash
   busybox sh standalone/freent.sh
   ```

## Creating a Portable FreeNT

To create a fully portable FreeNT installation:

### Windows

1. Create portable Python:
   ```cmd
   mkdir portable-freent
   cd portable-freent
   ```

2. Download and extract portable Python:
   ```cmd
   curl -L https://www.python.org/ftp/python/3.11.0/python-3.11.0-amd64.zip -o python.zip
   tar -xf python.zip
   ```

3. Copy FreeNT files:
   ```cmd
   xcopy /E /I path\to\FreeNT\src src
   xcopy /E /I path\to\FreeNT\standalone standalone
   ```

4. Install dependencies:
   ```cmd
   python\python.exe -m pip install --target=standalone/lib -r requirements.txt
   ```

5. Run FreeNT:
   ```cmd
   standalone\freent.bat
   ```

### Unix/Linux

1. Create portable Python:
   ```bash
   mkdir -p portable-freent
   cd portable-freent
   ```

2. Download and extract portable Python:
   ```bash
   wget https://www.python.org/ftp/python/3.11.0/python-3.11.0-linux-x86_64-embed-amd64.zip
   unzip python-3.11.0-linux-x86_64-embed-amd64.zip
   ```

3. Copy FreeNT files:
   ```bash
   cp -r path/to/FreeNT/src .
   cp -r path/to/FreeNT/standalone .
   ```

4. Install dependencies:
   ```bash
   ./python/bin/python3 -m pip install --target=standalone/lib -r requirements.txt
   ```

5. Make scripts executable:
   ```bash
   chmod -R +x standalone/
   ```

6. Run FreeNT:
   ```bash
   ./standalone/freent.sh
   ```

## Environment Variables

FreeNT recognizes the following environment variables:

- `FREENT_HOME` - Root directory of FreeNT installation
- `FREENT_CONFIG` - Path to configuration file (default: `$FREENT_HOME/etc/freent.json`)
- `FREENT_LOG_LEVEL` - Logging level (DEBUG, INFO, WARNING, ERROR)
- `FREENT_TOOLCHAIN` - Toolchain type (gnu or gnu-less)

## Troubleshooting

### Python not found

**Error**: `Error: Python is required to run FreeNT`

**Solution**:
1. Install Python system-wide
2. Place portable Python in `standalone/python/`
3. Set PYTHON environment variable

### Module not found

**Error**: `ModuleNotFoundError: No module named 'textual'`

**Solution**:
1. Install dependencies: `pip install -r requirements.txt`
2. For portable: `python -m pip install --target=standalone/lib -r requirements.txt`

### Shell script permission denied

**Error**: `./freent.sh: Permission denied`

**Solution**:
```bash
chmod +x standalone/*.sh
```

### BusyBox compatibility issues

**Error**: Script fails with BusyBox

**Solution**:
1. Ensure BusyBox has required applets
2. Try with `busybox ash` instead of `busybox sh`
3. Check BusyBox version: `busybox | head -1`

## Performance Considerations

For best performance in standalone mode:
- Use portable Python with all required modules pre-installed
- Pre-compile Python bytecode: `python -m compileall .`
- Use `--no-site-packages` flag if not using system Python
- Consider using PyPy for better performance

## Security Considerations

When running in standalone mode:
- Ensure portable Python is from a trusted source
- Verify checksums of downloaded files
- Don't run as root unless necessary
- Keep Python and dependencies updated
