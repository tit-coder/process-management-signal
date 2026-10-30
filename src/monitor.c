#define _POSIX_C_SOURCE 200809L
#include "monitor.h"
#include "process_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char* get_process_state_string(pid_t pid) {
    if (pid <= 0) {
        return "UNKNOWN";
    }

    if (!is_process_alive(pid)) {
        return "TERMINATED";
    }

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return "TERMINATED";
    }

    char line[256];
    char state_char = '\0';
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "State:", 6) == 0) {
            char *p = line + 6;
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            state_char = *p;
            break;
        }
    }
    fclose(fp);

    switch (state_char) {
        case 'R':
        case 'S':
        case 'D':
            return "RUNNING";
        case 'T':
        case 't':
            return "STOPPED";
        case 'Z':
        case 'X':
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

void show_process_info(pid_t pid) {
    printf("\nPID       : %d\n", pid);
    printf("Process   : worker\n");
    printf("Status    : %s\n\n", get_process_state_string(pid));
}

void show_menu(void) {
    printf("----------------------------------------\n");
    printf("1. Suspend Process\n");
    printf("2. Resume Process\n");
    printf("3. Show Process Status\n");
    printf("4. Terminate Process\n");
    printf("5. Exit\n");
    printf("----------------------------------------\n");
    printf("Enter choice: ");
    fflush(stdout);
}

void show_status(pid_t pid) {
    printf("\n[Process Status Monitor]\n");
    printf("PID       : %d\n", pid);
    printf("Process   : worker\n");
    printf("Status    : %s\n", get_process_state_string(pid));
    printf("----------------------------------------\n");
}

int get_user_choice(void) {
    char buffer[64];

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        /* End of file reached (e.g. piped input in automated tests) */
        return 5;
    }

    /* Trim leading spaces */
    char *start = buffer;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '\0') {
        return -1; /* Empty input */
    }

    /* Parse number */
    char *endptr = NULL;
    long val = strtol(start, &endptr, 10);

    /* Trim trailing spaces */
    while (endptr && isspace((unsigned char)*endptr)) {
        endptr++;
    }

    if (endptr && *endptr != '\0') {
        return -1; /* Contains trailing non-digits */
    }

    return (int)val;
}
