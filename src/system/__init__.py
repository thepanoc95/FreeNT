# FreeNT System Component
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
System identity and component management for FreeNT.
This module handles system transformation, identity modification,
and replacement of Windows components with Vital-Utilities.
"""

from .identity import SystemIdentity, modify_system_identity
from .components import ComponentManager, replace_explorer, restore_explorer
from .vital import VitalUtilitiesManager, ensure_vital_utilities

__all__ = [
    "SystemIdentity",
    "modify_system_identity",
    "ComponentManager",
    "replace_explorer",
    "restore_explorer",
    "VitalUtilitiesManager",
    "ensure_vital_utilities",
]
