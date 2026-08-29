# Native POSIX compatibility layer

`ntposix.c` is a self-contained POSIX-shaped descriptor layer for NT Native
Mode.  It deliberately calls `ntdll.dll` directly, avoiding a dependency on
Win32.  The layer now supports `open`, `close`, `read`, `write`, `fstat`,
`lseek`, and `dup`, including POSIX-style negative errno results.

The existing `autobuild` script produces small x86 or x64 native test hosts.
The source remains single-file so it can be used while the POSIX personality
is bootstrapping; the next stage can link the same API surface to FreeDLL and
NTTTY for terminal-backed descriptors.
