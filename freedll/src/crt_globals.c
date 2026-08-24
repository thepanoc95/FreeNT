/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/crt_globals.c
 * PURPOSE:         CRT global data exports
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* CRT global variables */
/* These are used by the CRT startup code and can be accessed by applications */

int   __argc = 0;           /* Number of command line arguments */
char **__argv = NULL;       /* Argument strings (ANSI) */
WCHAR **__wargv = NULL;     /* Argument strings (Unicode) */
char **__environ = NULL;    /* Environment strings */
char **__p__environ = NULL; /* Pointer to environment strings */

/* FLS index for the process */
ULONG_PTR fls_index = 0;

/* DLL main entry name pointer */
PSTR __dll_main_name = "freedll.dll";
