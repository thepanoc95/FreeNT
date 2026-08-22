# FreeNT Setup Script
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Setup script for FreeNT.
This script is used to build and install FreeNT as a Python package.
"""

import os
import sys
from setuptools import setup, find_packages

# Read version from __init__.py
version = "0.1.0"
try:
    with open("src/__init__.py", "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("__version__"):
                version = line.split("=")[1].strip().strip('"').strip("'")
                break
except Exception:
    pass

# Read requirements
requirements = []
try:
    with open("requirements.txt", "r", encoding="utf-8") as f:
        requirements = [
            line.strip() for line in f 
            if line.strip() and not line.strip().startswith("#")
        ]
except Exception:
    pass

# Read long description from README
long_description = ""
try:
    with open("README.md", "r", encoding="utf-8") as f:
        long_description = f.read()
except Exception:
    pass

setup(
    name="FreeNT",
    version=version,
    description="An Free and opensource userland for Windows 11 (with 32-bit support)",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Panoc95",
    author_email="",
    url="https://github.com/thepanoc95/FreeNT",
    license="BSD 3-Clause",
    classifiers=[
        "Development Status :: 1 - Planning",
        "Intended Audience :: Developers",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: BSD License",
        "Operating System :: Microsoft :: Windows :: Windows 10",
        "Operating System :: Microsoft :: Windows :: Windows 11",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: System :: Operating System",
        "Topic :: System :: Systems Administration",
        "Topic :: Utilities",
    ],
    python_requires=">=3.8",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    package_data={
        "": ["*.json", "*.yaml", "*.yml", "*.toml"],
    },
    install_requires=requirements,
    entry_points={
        "console_scripts": [
            "freent-login=FreeNT.login_manager.login_app:main",
            "freent-winget=FreeNT.winget_wrapper.winget:main",
            "freent=FreeNT.cli:main",
        ],
    },
    project_urls={
        "Bug Reports": "https://github.com/thepanoc95/FreeNT/issues",
        "Source": "https://github.com/thepanoc95/FreeNT",
    },
)
