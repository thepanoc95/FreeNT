#include <curses.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
extern char *_pgmptr;
#endif

/* ---- Configuration ---- */
#define MAX_DISKS       32
#define MAX_PATH_LEN    260
#define MAX_LINE_LEN    256

/* Installer configuration */
static struct {
    int  disk_number;
    char partition_style;   /* 'U' = UEFI/GPT, 'B' = BIOS/MBR */
    char wim_path[MAX_PATH_LEN];
    int  wim_index;
    int  step;
    int  freent_kernel;     /* When set, copy WinPE NT components instead of applying WIM */
} config = {0};

/* Disk information */
static struct {
    int  number;
    char model[128];
    unsigned long long size_bytes;
    int  is_removable;
} disks[MAX_DISKS];

static int num_disks = 0;

/* ---- Utility helpers ---- */

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

static int run_command(const char *cmd, char *output, size_t out_size) {
    FILE *fp;
    int status = 0;
    char line[MAX_LINE_LEN];

    if (output) {
        output[0] = '\0';
    }

    fp = _popen(cmd, "r");
    if (!fp) return -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (output && out_size > 1) {
            size_t len = strlen(line);
            if (len > out_size - 1) len = out_size - 1;
            memcpy(output + strlen(output), line, len);
            output[strlen(output) + len] = '\0';
            out_size -= len;
        }
    }

    status = _pclose(fp);
    return status;
}

/* ---- Disk detection ---- */

static void detect_disks(void) {
    FILE *fp;
    char line[MAX_LINE_LEN];
    int current_disk = -1;

    num_disks = 0;

    fp = _popen("wmic diskdrive get Index,Model,Size,RemovableSystem 2>nul", "r");
    if (!fp) return;

    /* Skip the first two lines (headers) */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) != NULL && num_disks < MAX_DISKS) {
        trim_newline(line);
        if (strlen(line) == 0) continue;

        /* Simple parsing - wmic aligns columns with spaces */
        if (line[0] >= '0' && line[0] <= '9' && strlen(line) < 3) {
            current_disk = atoi(line);
        } else if (current_disk >= 0 && strlen(line) > 0) {
            /* This is part of the disk info */
            disks[num_disks].number = current_disk;

            char *p = line;
            /* Find model - skip leading spaces */
            while (*p == ' ') p++;
            strncpy(disks[num_disks].model, p, sizeof(disks[num_disks].model) - 1);

            num_disks++;
            current_disk = -1;
        }
    }
    _pclose(fp);

    /* Fallback: if wmic didn't work, try diskpart */
    if (num_disks == 0) {
        fp = _popen("echo list disk | diskpart 2>nul", "r");
        if (!fp) return;

        while (fgets(line, sizeof(line), fp) != NULL) {
            trim_newline(line);
            /* Lines like "  0  Online  500 GB  *" */
            if (strncmp(line, "  ", 2) == 0) {
                int n;
                if (sscanf(line, "  %d", &n) == 1) {
                    if (num_disks < MAX_DISKS) {
                        disks[num_disks].number = n;
                        snprintf(disks[num_disks].model, sizeof(disks[num_disks].model),
                                 "Disk %d", n);
                        num_disks++;
                    }
                }
            }
        }
        _pclose(fp);
    }
}

/* ---- DISKPART script execution ---- */

static int run_diskpart(const char *commands) {
    char script[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 2];
    int ret = 0;
    const char *temp_dir = getenv("TEMP");
    if (temp_dir == NULL) temp_dir = "X:\\Temp";

    snprintf(script, sizeof(script),
             "echo select disk %d > %s\\dp_script.txt",
             config.disk_number, temp_dir);

    char *p = (char *)commands;
    while (*p) {
        const char *end = strchr(p, '\n');
        int len = end ? (int)(end - p) : (int)strlen(p);
        if (len > 0 && p[len-1] == '\r') len--;
        if (len > 0) {
            snprintf(cmd, sizeof(cmd), "echo %.*s >> %s\\dp_script.txt",
                     len, p, temp_dir);
            run_command(cmd, NULL, 0);
        }
        if (!end) break;
        p = end + 1;
    }

    snprintf(cmd, sizeof(cmd), "diskpart /s %s\\dp_script.txt", temp_dir);
    ret = run_command(cmd, NULL, 0);

    snprintf(cmd, sizeof(cmd), "%s\\dp_script.txt", temp_dir);
    remove(cmd);

    return ret;
}

/* ---- TUI rendering ---- */

static void draw_header(const char *title) {
    int i;
    move(0, 0);
    attron(A_REVERSE);
    for (i = 0; i < COLS; i++) addch(' ');
    mvaddnstr(0, 2, title, COLS - 4);
    attroff(A_REVERSE);
}

static void draw_footer(const char *msg) {
    int i;
    move(LINES - 1, 0);
    attron(A_REVERSE);
    for (i = 0; i < COLS; i++) addch(' ');
    mvaddnstr(LINES - 1, 2, msg, COLS - 4);
    attroff(A_REVERSE);
}

static void draw_separator(int y) {
    int i;
    move(y, 0);
    attron(A_DIM);
    for (i = 0; i < COLS; i++) addch('-');
    attroff(A_DIM);
}

static void wait_for_key(const char *msg) {
    move(LINES - 3, 0);
    clrtoeol();
    attron(A_BOLD);
    mvaddstr(LINES - 3, 2, msg);
    attroff(A_BOLD);
    refresh();
    /* Wait for any key */
    int c = getch();
    (void)c;
    move(LINES - 3, 0);
    clrtoeol();
}

/* ---- Screens ---- */

static void screen_disk_selection(void) {
    int i, highlight = 0;
    int running = 1;

    while (running) {
        clear();
        draw_header("FreeNT Installer - Disk Selection");

        mvaddstr(2, 2, "Select the disk to install FreeNT to:");
        draw_separator(4);

        for (i = 0; i < num_disks && i < 15; i++) {
            if (i == highlight) {
                attron(A_REVERSE);
                mvprintw(5 + i, 2, "> Disk %d - %s", disks[i].number, disks[i].model);
                attroff(A_REVERSE);
            } else {
                mvprintw(5 + i, 2, "  Disk %d - %s", disks[i].number, disks[i].model);
            }
        }

        draw_separator(LINES - 3);
        mvaddstr(LINES - 2, 2, "UP/DOWN to select, ENTER to confirm, Q to quit");

        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                if (highlight > 0) highlight--;
                break;
            case KEY_DOWN:
                if (highlight < num_disks - 1 && highlight < 14) highlight++;
                break;
            case '\n':
            case KEY_ENTER:
                config.disk_number = disks[highlight].number;
                running = 0;
                break;
            case 'q':
            case 'Q':
                exit(0);
        }
    }
}

static void screen_partition_style(void) {
    int choice = (config.partition_style == 'B') ? 1 : 0;
    int running = 1;

    while (running) {
        clear();
        draw_header("FreeNT Installer - Partition Style");

        mvaddstr(2, 2, "Select partition style based on your firmware:");
        mvaddstr(4, 2, "UEFI-based systems should use GPT.");
        mvaddstr(5, 2, "Legacy BIOS systems should use MBR.");

        draw_separator(7);

        if (choice == 0) {
            attron(A_REVERSE);
            mvprintw(8, 4, "[*] UEFI (GPT) -- Recommended");
            attroff(A_REVERSE);
        } else {
            mvprintw(8, 4, "    UEFI (GPT) -- Recommended");
        }

        if (choice == 1) {
            attron(A_REVERSE);
            mvprintw(10, 4, "[*] BIOS (MBR)");
            attroff(A_REVERSE);
        } else {
            mvprintw(10, 4, "    BIOS (MBR)");
        }

        draw_separator(LINES - 3);
        mvaddstr(LINES - 2, 2, "UP/DOWN to select, ENTER to confirm, Q to quit");

        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                choice = 0;
                break;
            case KEY_DOWN:
                choice = 1;
                break;
            case '\n':
            case KEY_ENTER:
                config.partition_style = (choice == 0) ? 'U' : 'B';
                running = 0;
                break;
            case 'q':
            case 'Q':
                exit(0);
        }
    }
}

static void screen_dism_config(void) {
    /* Skip WIM configuration if freent_kernel mode is active */
    if (config.freent_kernel) {
        return;
    }

    char input[MAX_PATH_LEN];

    while (1) {
        clear();
        draw_header("FreeNT Installer - Image Configuration");

        mvprintw(2, 2, "WIM file path: %s", config.wim_path);
        mvprintw(3, 2, "Image index:   %d", config.wim_index);
        mvprintw(5, 2, "Enter WIM path (press Enter to keep current, or type new path)");
        mvprintw(6, 2, "Or 'scan' to auto-detect install.wim");
        mvprintw(7, 2, "Or 'skip' to continue without DISM apply");
        mvprintw(8, 2, "> ");

        echo();
        curs_set(1);
        /* Get string from user */
        move(8, 4);
        clrtoeol();
        getnstr(input, MAX_PATH_LEN - 1);
        echo();
        curs_set(0);

        trim_newline(input);

        if (strcasecmp(input, "scan") == 0) {
            /* Auto-detect */
            const char *drives[] = {"D:\\sources\\install.wim",
                                    "E:\\sources\\install.wim",
                                    "F:\\sources\\install.wim",
                                    "X:\\sources\\install.wim",
                                    NULL};
            for (int i = 0; drives[i]; i++) {
                FILE *fp = fopen(drives[i], "rb");
                if (fp) {
                    fclose(fp);
                    strncpy(config.wim_path, drives[i], MAX_PATH_LEN - 1);
                    config.wim_path[MAX_PATH_LEN - 1] = '\0';
                    mvprintw(10, 2, "Found: %s", config.wim_path);
                    refresh();
                    break;
                }
            }
            if (config.wim_path[0] == '\0') {
                mvprintw(10, 2, "No install.wim found on any drive.");
                refresh();
                wait_for_key("Press any key to continue...");
            }
            continue;
        } else if (strcasecmp(input, "skip") == 0) {
            config.wim_path[0] = '\0';
            break;
        } else if (strlen(input) > 0) {
            strncpy(config.wim_path, input, MAX_PATH_LEN - 1);
            config.wim_path[MAX_PATH_LEN - 1] = '\0';
        }

        break;
    }

    /* Get image index */
    while (1) {
        clear();
        draw_header("FreeNT Installer - Image Index");

        mvprintw(2, 2, "WIM file: %s", config.wim_path);

        if (config.wim_path[0] != '\0') {
            char cmd[MAX_PATH_LEN * 2];
            char output[4096];
            snprintf(cmd, sizeof(cmd), "dism /get-imageinfo /imagefile:\"%s\" 2>nul", config.wim_path);
            run_command(cmd, output, sizeof(output));

            mvaddstr(4, 2, "Available images:");
            /* Parse and display image indices */
            /* Simplified - just show the DISM output */
            int y = 6;
            char *line = strtok(output, "\n");
            while (line && y < LINES - 5) {
                mvprintw(y++, 4, "%.70s", line);
                line = strtok(NULL, "\n");
            }
        }

        mvprintw(LINES - 3, 2, "Enter image index number:");
        mvprintw(LINES - 2, 2, "> ");

        echo();
        curs_set(1);
        move(LINES - 2, 4);
        clrtoeol();
        getnstr(input, 16);
        echo();
        curs_set(0);

        int idx = atoi(input);
        if (idx > 0) {
            config.wim_index = idx;
            break;
        }
        mvaddstr(LINES - 2, 2, "Invalid index. Try again.");
        refresh();
        getch();
    }
}

static void screen_summary(void) {
    clear();
    draw_header("FreeNT Installer - Installation Summary");

    int y = 2;
    mvprintw(y++, 2, "Installation Summary:");
    mvprintw(y++, 2, "  Target Disk:     %d", config.disk_number);
    mvprintw(y++, 2, "  Partition Style: %s",
             (config.partition_style == 'U') ? "UEFI (GPT)" : "BIOS (MBR)");
    if (config.freent_kernel) {
        mvprintw(y++, 2, "  Mode:            WinPE kernel copy mode");
        mvaddstr(y++, 2, "  (Will copy WinPE NT components to C:\\WNT");
        mvaddstr(y++, 2, "   and install FreeNT + NativeShell replacements)");
    } else {
        mvprintw(y++, 2, "  WIM File:        %s", config.wim_path);
        mvprintw(y++, 2, "  Image Index:     %d", config.wim_index);
    }
    y++;
    mvprintw(y++, 2, "WARNING: All data on the target disk will be erased!");
    y++;
    draw_separator(y);
    y++;
    mvprintw(y++, 2, "Press ENTER to begin installation, or Q to abort.");

    refresh();

    int ch = getch();
    if (ch == 'q' || ch == 'Q') exit(0);
}

static void screen_progress(const char *message, int *status) {
    clear();
    draw_header("FreeNT Installer - Progress");

    mvprintw(2, 2, "Installing FreeNT...");
    mvprintw(4, 2, "%s", message);

    draw_separator(LINES - 3);
    draw_footer("Please do not remove the installation media");

    refresh();

    if (status) *status = 1;
}

/* ---- Installation steps ---- */

static int execute_installation(void) {
    char cmd[MAX_PATH_LEN * 2];
    char output[4096];
    int step = 0;

    screen_progress("Step 1: Partitioning disk...", &step);
    step = 1;
    refresh();

    /* Step 1: Partition */
    if (config.partition_style == 'U') {
        run_diskpart(
            "clean\n"
            "convert gpt\n"
            "create partition efi size=100\n"
            "format quick fs=fat32 label=System\n"
            "assign letter=S\n"
            "create partition msr size=16\n"
            "create partition primary\n"
            "format quick fs=ntfs label=FreeNT\n"
            "assign letter=C\n"
        );
    } else {
        run_diskpart(
            "clean\n"
            "convert mbr\n"
            "create partition primary\n"
            "format quick fs=ntfs label=FreeNT\n"
            "assign letter=C\n"
            "active\n"
        );
    }

    /* Step 2: Apply Windows image or copy WinPE NT components */
    if (config.freent_kernel) {
        screen_progress("Step 2: Copying WinPE NT components...", &step);
        step = 2;
        refresh();

        /* Copy the WinPE Windows directory to the target as WNT */
        run_command("xcopy /E /I /Y X:\\Windows C:\\WNT 2>nul", output, sizeof(output));

        /* Copy FreeNT components into the WinPE image */
        char installer_dir[MAX_PATH_LEN];
        if (_pgmptr && strlen(_pgmptr) > 0) {
            _fullpath(installer_dir, _pgmptr, MAX_PATH_LEN);
            char *slash = strrchr(installer_dir, '\\');
            if (slash) *slash = '\0';
        } else {
            strncpy(installer_dir, ".\\", MAX_PATH_LEN - 1);
            installer_dir[MAX_PATH_LEN - 1] = '\0';
        }

        /* Copy FreeNT DLLs and executables into System32 */
        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\freedll.dll\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\ntdylib.dll\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\freent.exe\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\liberty.exe\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        /* Copy NativeShell if available */
        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\native.exe\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);
    } else if (config.wim_path[0] != '\0') {
        screen_progress("Step 2: Applying Windows image...", &step);
        step = 2;
        refresh();

        snprintf(cmd, sizeof(cmd),
                 "dism /apply-image /imagefile:\"%s\" /index:%d /applydir:C:\\",
                 config.wim_path, config.wim_index);
        run_command(cmd, output, sizeof(output));
    } else {
        /* No WIM and no kernel copy — skip image step */
        screen_progress("Step 2: Skipping image application", &step);
        step = 2;
        refresh();
    }

    /* Step 3: Rename Windows directory (skip if freent_kernel - already C:\WNT) */
    if (!config.freent_kernel) {
        screen_progress("Step 3: Renaming Windows directory...", &step);
        step = 3;
        refresh();

        if (GetFileAttributes("C:\\Windows") != INVALID_FILE_ATTRIBUTES) {
            if (MoveFileEx("C:\\Windows", "C:\\WNT",
                           MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                /* Success */
            } else {
                /* Try via diskpart/rename fallback */
                run_command("rename C:\\Windows WNT", output, sizeof(output));
            }
        }
    }

    /* Step 4: Remove proprietary Microsoft components or replace with FreeNT */
    if (!config.freent_kernel) {
        screen_progress("Step 4: Removing Microsoft components...", &step);
        step = 4;
        refresh();

        const char *to_remove_files[] = {
        "C:\\WNT\\System32\\smss.exe",
        "C:\\WNT\\System32\\winlogon.exe",
        "C:\\WNT\\System32\\csrss.exe",
        "C:\\WNT\\System32\\services.exe",
        "C:\\WNT\\System32\\lsass.exe",
        "C:\\WNT\\System32\\wininit.exe",
        "C:\\WNT\\System32\\splwow64.exe",
        "C:\\WNT\\System32\\dwm.exe",
        "C:\\WNT\\System32\\dwmcore.dll",
        "C:\\WNT\\System32\\dwmrender.dll",
        "C:\\WNT\\System32\\dwmapi.dll",
        "C:\\WNT\\System32\\dwmcomppositing.dll",
        "C:\\WNT\\System32\\Win32k.sys",
        "C:\\WNT\\System32\\cdd.dll",
        "C:\\WNT\\System32\\kernel32.dll",
        "C:\\WNT\\System32\\user32.dll",
        "C:\\WNT\\System32\\gdi32.dll",
        "C:\\WNT\\System32\\winspool.drv",
        "C:\\WNT\\System32\\imm32.dll",
        "C:\\WNT\\System32\\shell32.dll",
        "C:\\WNT\\System32\\shlwapi.dll",
        "C:\\WNT\\System32\\uxtheme.dll",
        "C:\\WNT\\System32\\comdlg32.dll",
        "C:\\WNT\\System32\\comctl32.dll",
        "C:\\WNT\\System32\\shdocvw.dll",
        "C:\\WNT\\System32\\ole32.dll",
        "C:\\WNT\\System32\\oleaut32.dll",
        "C:\\WNT\\System32\\rpcrt4.dll",
        "C:\\WNT\\System32\\secur32.dll",
        "C:\\WNT\\System32\\wininet.dll",
        "C:\\WNT\\System32\\urlmon.dll",
        "C:\\WNT\\System32\\winhttp.dll",
        "C:\\WNT\\System32\\winmm.dll",
        "C:\\WNT\\System32\\winmmcfg.dll",
        "C:\\WNT\\System32\\powrprof.dll",
        "C:\\WNT\\explorer.exe",
        "C:\\WNT\\System32\\explorer.exe",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img1.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img2.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img3.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img4.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img5.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img6.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img7.jpg",
        "C:\\WNT\\Web\\Wallpaper\\Windows\\img8.jpg",
        "C:\\WNT\\Resources\\Themes\\aero.theme",
        "C:\\WNT\\Resources\\Themes\\basic.theme",
        "C:\\WNT\\Resources\\Themes\\highcontrast.theme",
        "C:\\WNT\\Resources\\Themes\\luna.theme",
        "C:\\WNT\\Fonts\\arial.ttf",
        "C:\\WNT\\Fonts\\ariali.ttf",
        "C:\\WNT\\Fonts\\arialbd.ttf",
        "C:\\WNT\\Fonts\\arialbi.ttf",
        "C:\\WNT\\Fonts\\cour.ttf",
        "C:\\WNT\\Fonts\\couri.ttf",
        "C:\\WNT\\Fonts\\courbd.ttf",
        "C:\\WNT\\Fonts\\courbi.ttf",
        "C:\\WNT\\Fonts\\times.ttf",
        "C:\\WNT\\Fonts\\timesi.ttf",
        "C:\\WNT\\Fonts\\timesbd.ttf",
        "C:\\WNT\\Fonts\\timesbi.ttf",
        "C:\\WNT\\System32\\spoolss.dll",
        "C:\\WNT\\System32\\spoolsv.exe",
        "C:\\WNT\\System32\\edgehtml.dll",
        "C:\\WNT\\System32\\Edge\\*",
        "C:\\WNT\\SystemApps\\MicrosoftEdge_*",
        "C:\\WNT\\System32\\onedrive.exe",
        "C:\\WNT\\System32\\OneDriveSetup.exe",
        "C:\\WNT\\System32\\mrt100.dll",
        "C:\\WNT\\System32\\korwbrkr.dll",
        "C:\\WNT\\Program Files\\Windows Defender\\*",
        "C:\\WNT\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windows Defender.lnk",
        "C:\\WNT\\Help\\*",
        "C:\\WNT\\Support\\*",
        "C:\\WNT\\System32\\1025\\*",
        "C:\\WNT\\System32\\1028\\*",
        "C:\\WNT\\System32\\1029\\*",
        "C:\\WNT\\System32\\1030\\*",
        "C:\\WNT\\System32\\1031\\*",
        "C:\\WNT\\System32\\1032\\*",
        "C:\\WNT\\System32\\1033\\*",
        "C:\\WNT\\System32\\1034\\*",
        "C:\\WNT\\System32\\1035\\*",
        "C:\\WNT\\System32\\1036\\*",
        "C:\\WNT\\System32\\1037\\*",
        "C:\\WNT\\System32\\1038\\*",
        "C:\\WNT\\System32\\1040\\*",
        "C:\\WNT\\System32\\1041\\*",
        "C:\\WNT\\System32\\1042\\*",
        "C:\\WNT\\System32\\1043\\*",
        "C:\\WNT\\System32\\1044\\*",
        "C:\\WNT\\System32\\1045\\*",
        "C:\\WNT\\System32\\1046\\*",
        "C:\\WNT\\System32\\1049\\*",
        "C:\\WNT\\System32\\1050\\*",
        "C:\\WNT\\System32\\1051\\*",
        "C:\\WNT\\System32\\1052\\*",
        "C:\\WNT\\System32\\1053\\*",
        "C:\\WNT\\System32\\1055\\*",
        "C:\\WNT\\System32\\1056\\*",
        "C:\\WNT\\System32\\1057\\*",
        "C:\\WNT\\System32\\1058\\*",
        "C:\\WNT\\System32\\1060\\*",
        "C:\\WNT\\System32\\1061\\*",
        "C:\\WNT\\System32\\1062\\*",
        "C:\\WNT\\System32\\1063\\*",
        "C:\\WNT\\System32\\1066\\*",
        "C:\\WNT\\System32\\1067\\*",
        "C:\\WNT\\System32\\1071\\*",
        "C:\\WNT\\System32\\1074\\*",
        "C:\\WNT\\System32\\1078\\*",
        "C:\\WNT\\System32\\1079\\*",
        "C:\\WNT\\System32\\11239\\*",
        "C:\\WNT\\System32\\2052\\*",
        "C:\\WNT\\System32\\2057\\*",
        "C:\\WNT\\System32\\2058\\*",
        "C:\\WNT\\System32\\2064\\*",
        "C:\\WNT\\System32\\2066\\*",
        "C:\\WNT\\System32\\2070\\*",
        "C:\\WNT\\System32\\2074\\*",
        "C:\\WNT\\System32\\2107\\*",
        "C:\\WNT\\System32\\2108\\*",
        "C:\\WNT\\System32\\2117\\*",
        "C:\\WNT\\System32\\2119\\*",
        "C:\\WNT\\System32\\2120\\*",
        "C:\\WNT\\System32\\2121\\*",
        "C:\\WNT\\System32\\2122\\*",
        "C:\\WNT\\System32\\2149\\*",
        "C:\\WNT\\System32\\3076\\*",
        "C:\\WNT\\System32\\3082\\*",
        "C:\\WNT\\System32\\3097\\*",
        "C:\\WNT\\System32\\3098\\*",
        "C:\\WNT\\System32\\3099\\*",
        "C:\\WNT\\System32\\3100\\*",
        "C:\\WNT\\System32\\3101\\*",
        "C:\\WNT\\System32\\3102\\*",
        "C:\\WNT\\System32\\3103\\*",
        "C:\\WNT\\System32\\3104\\*",
        "C:\\WNT\\System32\\3105\\*",
        "C:\\WNT\\System32\\3106\\*",
        "C:\\WNT\\System32\\3107\\*",
        "C:\\WNT\\System32\\3108\\*",
        "C:\\WNT\\System32\\3109\\*",
        "C:\\WNT\\System32\\3110\\*",
        "C:\\WNT\\System32\\32767\\*",
    };

    for (int i = 0; to_remove_files[i]; i++) {
        snprintf(cmd, sizeof(cmd),
                 "del /f /q \"%s\" 2>nul", to_remove_files[i]);
        run_command(cmd, NULL, 0);
    }

    /* Remove entire bloat directories */
    const char *to_remove_dirs[] = {
        "C:\\WNT\\System32\\AppLocker",
        "C:\\WNT\\System32\\AppReadiness",
        "C:\\WNT\\System32\\AppX",
        "C:\\WNT\\System32\\GroupPolicy",
        "C:\\WNT\\System32\\GroupPolicyUsers",
        "C:\\WNT\\System32\\NlsData",
        "C:\\WNT\\System32\\speech_xtk_resources_1",
        "C:\\WNT\\System32\\Speech",
        "C:\\WNT\\System32\\spool",
        "C:\\WNT\\System32\\WaaSMedic",
        "C:\\WNT\\System32\\WDI",
        "C:\\WNT\\System32\\WebTheme",
        "C:\\WNT\\System32\\winevt\\Logs",
        "C:\\WNT\\System32\\winevt\\ManifestCache",
        "C:\\WNT\\System32\\wseo",
        "C:\\WNT\\AppReadiness",
        "C:\\WNT\\AppModelUnlock",
        "C:\\WNT\\AssertInfo",
        "C:\\WNT\\Boot",
        "C:\\WNT\\Cursors",
        "C:\\WNT\\DigitalLocker",
        "C:\\WNT\\Documents and Settings",
        "C:\\WNT\\eHome",
        "C:\\WNT\\ELAMBKGO",
        "C:\\WNT\\Embedding",
        "C:\\WNT\\Fonts",
        "C:\\WNT\\Globalization",
        "C:\\WNT\\Help",
        "C:\\WNT\\IdentityCRL",
        "C:\\WNT\\ImmersiveControlPanel",
        "C:\\WNT\\InboxApps",
        "C:\\WNT\\Lyrics",
        "C:\\WNT\\Media",
        "C:\\WNT\\Memory",
        "C:\\WNT\\Microsoft",
        "C:\\WNT\\Microsoft.NET",
        "C:\\WNT\\OfflineCache",
        "C:\\WNT\\PerfLogs",
        "C:\\WNT\\Performance",
        "C:\\WNT\\PLA",
        "C:\\WNT\\PolicyCoverage",
        "C:\\WNT\\Prefetch",
        "C:\\WNT\\RemotePackages",
        "C:\\WNT\\Resources",
        "C:\\WNT\\SCHEMAPFX",
        "C:\\WNT\\ServiceProfiles",
        "C:\\WNT\\Setup",
        "C:\\WNT\\Share",
        "C:\\WNT\\software",
        "C:\\WNT\\Spinner",
        "C:\\WNT\\Speech",
        "C:\\WNT\\Spool",
        "C:\\WNT\\ssdfdata",
        "C:\\WNT\\system",
        "C:\\WNT\\System32\\AutoUpdate",
        "C:\\WNT\\System32\\LogFiles",
        "C:\\WNT\\System32\\LogPolicy",
        "C:\\WNT\\System32\\ManifestCache",
        "C:\\WNT\\System32\\mui",
        "C:\\WNT\\System32\\NlsData",
        "C:\\WNT\\System32\\Remotefile",
        "C:\\WNT\\System32\\restore",
        "C:\\WNT\\System32\\setup",
        "C:\\WNT\\System32\\Sysprep",
        "C:\\WNT\\System32\\tasks",
        "C:\\WNT\\System32\\Temp",
        "C:\\WNT\\System32\\winevt",
        "C:\\WNT\\System32\\WineShellExt",
        "C:\\WNT\\System32\\WinSxS",
        "C:\\WNT\\System32\\wt",
        "C:\\WNT\\System32\\WwanSvc",
        "C:\\WNT\\SystemResources",
        "C:\\WNT\\SysWOW64",
        "C:\\WNT\\Temp",
        "C:\\WNT\\WCM",
        "C:\\WNT\\WDI",
        "C:\\WNT\\WDT",
        "C:\\WNT\\Web",
            NULL
    };

    for (int i = 0; to_remove_dirs[i]; i++) {
        snprintf(cmd, sizeof(cmd),
                 "rmdir /s /q \"%s\" 2>nul", to_remove_dirs[i]);
        run_command(cmd, NULL, 0);
    }

    } /* end if (!config.freent_kernel) */

    /* Step 5: Apply registry patches */
    screen_progress("Step 5: Applying FreeNT registry patches...", &step);
    step = 5;
    refresh();

    /* Load the SYSTEM hive from the target so we can patch it offline */
    run_command("reg load HKLM\\FreeNT_System C:\\WNT\\System32\\config\\SYSTEM 2>nul", output, sizeof(output));

    /* Apply each .reg file. Since the hive is loaded under HKLM\FreeNT_System,
       we need to redirect HKEY_LOCAL_MACHINE paths to HKLM\FreeNT_System.
       We do this by creating temp copies of the .reg files with modified paths. */
    const char *reg_files[] = {
        ".\\patches\\osname.reg",
        ".\\patches\\smss.reg",
        ".\\patches\\systemroot.reg",
        NULL
    };

    const char *temp_dir = getenv("TEMP");
    if (temp_dir == NULL) temp_dir = "X:\\Temp";

    for (int i = 0; reg_files[i]; i++) {
        screen_progress("Step 5: Patching registry...", &step);
        refresh();

        char reg_path[MAX_PATH_LEN];
        char temp_path[MAX_PATH_LEN * 2];

        /* Resolve the full path to the reg file.
           The patches are in <repo>\patches\ relative to the installer executable.
           _pgmptr gives us the path to the .exe, so we go up one level for the repo root. */
        if (_pgmptr && strlen(_pgmptr) > 0) {
            /* Extract directory of the executable */
            char exe_dir[MAX_PATH_LEN];
            strncpy(exe_dir, _pgmptr, MAX_PATH_LEN - 1);
            exe_dir[MAX_PATH_LEN - 1] = '\0';
            char *slash = strrchr(exe_dir, '\\');
            if (slash) {
                *slash = '\0';
                snprintf(reg_path, sizeof(reg_path), "%s\\patches\\%s", exe_dir, reg_files[i] + 2);
            } else {
                snprintf(reg_path, sizeof(reg_path), "%s", reg_files[i]);
            }
        } else {
            snprintf(reg_path, sizeof(reg_path), "%s", reg_files[i]);
        }

        snprintf(temp_path, sizeof(temp_path), "%s\\freent_patch_%d.reg", temp_dir, i);

        /* Create a temp copy of the .reg file with HKLM -> HKLM\FreeNT_System */
        FILE *src = fopen(reg_path, "r");
        if (src) {
            FILE *dst = fopen(temp_path, "w");
            if (dst) {
                char line[512];
                while (fgets(line, sizeof(line), src) != NULL) {
                    /* Redirect HKEY_LOCAL_MACHINE -> HKLM\FreeNT_System */
                    if (strncmp(line, "[HKEY_LOCAL_MACHINE\\", 20) == 0) {
                        fprintf(dst, "[HKEY_LOCAL_MACHINE\\FreeNT_System\\%s", line + 20);
                    } else {
                        fputs(line, dst);
                    }
                }
                fclose(dst);
                fclose(src);

                /* Import the modified registry file into the loaded hive */
                snprintf(cmd, sizeof(cmd), "reg import \"%s\" 2>nul", temp_path);
                run_command(cmd, output, sizeof(output));
                remove(temp_path);
            } else {
                fclose(src);
            }
        }
    }

    /* Unload the SYSTEM hive */
    run_command("reg unload HKLM\\FreeNT_System 2>nul", NULL, 0);

    /* Step 6: Install FreeNT boot loader and NativeShell */
    screen_progress("Step 6: Installing FreeNT boot configuration...", &step);
    step = 6;
    refresh();

    /* Copy FreeNT components to the target */
    if (!config.freent_kernel) {
        /* Components were already applied via WIM; copy FreeNT replacements */
        char installer_dir[MAX_PATH_LEN];
        if (_pgmptr && strlen(_pgmptr) > 0) {
            _fullpath(installer_dir, _pgmptr, MAX_PATH_LEN);
            /* Strip the filename, keep the directory */
            char *slash = strrchr(installer_dir, '\\');
            if (slash) *slash = '\0';
        } else {
            strncpy(installer_dir, ".\\", MAX_PATH_LEN - 1);
            installer_dir[MAX_PATH_LEN - 1] = '\0';
        }

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\freedll.dll\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\ntdylib.dll\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\freent.exe\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\liberty.exe\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);
    }

    /* Install NativeShell (native.exe) */
    screen_progress("Step 7: Installing NativeShell...", &step);
    step = 7;
    refresh();

    {
        char installer_dir[MAX_PATH_LEN];
        if (_pgmptr && strlen(_pgmptr) > 0) {
            _fullpath(installer_dir, _pgmptr, MAX_PATH_LEN);
            char *slash = strrchr(installer_dir, '\\');
            if (slash) *slash = '\0';
        } else {
            strncpy(installer_dir, ".\\", MAX_PATH_LEN - 1);
            installer_dir[MAX_PATH_LEN - 1] = '\0';
        }

        snprintf(cmd, sizeof(cmd),
                 "xcopy /Y \"%s\\native.exe\" C:\\WNT\\System32\\ 2>nul",
                 installer_dir);
        run_command(cmd, NULL, 0);

        /* Apply NativeShell registry entries */
        snprintf(cmd, sizeof(cmd),
                 "reg import \"%s\\nativeshell\\install\\add.reg\" 2>nul",
                 installer_dir);
        run_command(cmd, output, sizeof(output));
    }

    /* Write boot configuration */
    if (config.partition_style == 'U') {
        /* UEFI: need to create BCD entry */
        run_command("bcdboot C:\\WNT /s S: /f UEFI /l en-us 2>nul", NULL, 0);
    } else {
        /* BIOS: write MBR boot sector */
        run_command("bootsect /nt60 C: /mbr 2>nul", NULL, 0);
        run_command("bootrec /fixmbr 2>nul", NULL, 0);
        run_command("bootrec /fixboot 2>nul", NULL, 0);
    }

    /* Final step: completion */
    clear();
    draw_header("FreeNT Installer - Complete");

    mvaddstr(3, 2, "Installation complete!");
    mvaddstr(5, 2, "The following was performed:");
    mvprintw(6, 4, "1. Disk partitioned (%s)",
             (config.partition_style == 'U') ? "UEFI/GPT" : "BIOS/MBR");
    if (config.freent_kernel) {
        mvaddstr(7, 4, "2. WinPE NT components copied to C:\\WNT");
        mvaddstr(8, 4, "3. FreeNT DLLs and executables installed");
        mvaddstr(9, 4, "4. NativeShell (native.exe) installed");
        mvaddstr(10, 4, "5. FreeNT registry patches applied");
        mvaddstr(11, 4, "6. FreeNT boot configuration written");
    } else {
        mvaddstr(7, 4, "2. Windows image applied to C:\\");
        mvaddstr(8, 4, "3. Windows directory renamed to C:\\WNT");
        mvaddstr(9, 4, "4. Microsoft components removed");
        mvaddstr(10, 4, "5. FreeNT registry patches applied");
        mvaddstr(11, 4, "6. FreeNT boot configuration written");
    }

    draw_separator(LINES - 5);
    mvaddstr(LINES - 3, 2, "You may now reboot into FreeNT.");

    draw_footer("Press any key to exit");

    refresh();
    getch();

    return 0;
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    /* Initialize default config */
    config.disk_number = 0;
    config.partition_style = 'U';
    config.wim_path[0] = '\0';
    config.wim_index = 1;
    config.step = 0;
    config.freent_kernel = 0;

    /* Allow config via command line */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--disk") == 0 && i + 1 < argc) {
            config.disk_number = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--bios") == 0) {
            config.partition_style = 'B';
        } else if (strcmp(argv[i], "--uefi") == 0) {
            config.partition_style = 'U';
        } else if (strcmp(argv[i], "--wim") == 0 && i + 1 < argc) {
            strncpy(config.wim_path, argv[++i], MAX_PATH_LEN - 1);
            config.wim_path[MAX_PATH_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "--index") == 0 && i + 1 < argc) {
            config.wim_index = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--freent-kernel") == 0) {
            config.freent_kernel = 1;
        }
    }

    /* Initialize curses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* Detect disks */
    detect_disks();

    /* Run the TUI wizard */
    screen_disk_selection();
    screen_partition_style();
    screen_dism_config();
    screen_summary();
    execute_installation();

    /* Cleanup */
    endwin();
    return 0;
}
