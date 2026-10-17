#define _POSIX_C_SOURCE 200809L
#include "monitor.h"
#include "process_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* ==============================================================================
 * ANSI Escape Codes - Dinh dang mau sac tren Terminal
 * ============================================================================== */
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_MAGENTA "\033[1;35m"

/**
 * @brief Kiem tra xem dau ra chuan (stdout) co phai la Terminal tuong tac (TTY) hay khong.
 * Neu la TTY -> Bat mau sac.
 * Neu la Pipe/Script tu dong (nhu khi chay bash test) -> Tat ma mau de tranh loi so khop chuoi.
 */
static int is_interactive_terminal(void) {
    return isatty(STDOUT_FILENO);
}

/**
 * @brief Doc file ao /proc/<PID>/status de lay trang thai thuc te tu Linux Kernel.
 * 
 * Uu diem cua phuong phap nay:
 * 1. Nhe, nhanh hon viec fork tien trinh phu goi lenh "ps".
 * 2. Chinh xac tuyet doi, phan biet duoc ro RUNNING (R/S) va STOPPED (T/t)
 *    ma ham kill(pid, 0) khong the phan biet duoc.
 */
const char* get_process_state_string(pid_t pid) {
    if (pid <= 0) {
        return "UNKNOWN";
    }

    /* Kiem tra xem tien trinh con ton tai khong qua kill(pid, 0) */
    if (!is_process_alive(pid)) {
        return "TERMINATED";
    }

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* Neu khong mo duoc file /proc, tien trinh co the vua ket thuc */
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

    /* Anh xa ky tu trang thai Linux Process sang ten chuan */
    switch (state_char) {
        case 'R': /* Running */
        case 'S': /* Sleeping */
        case 'D': /* Disk sleep */
            return "RUNNING";
        case 'T': /* Stopped by job control signal */
        case 't': /* Traced stop */
            return "STOPPED";
        case 'Z': /* Zombie */
        case 'X': /* Dead */
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Hien thi thong tin tom tat tien trinh tren dau moi luot hien thi menu.
 */
void show_process_info(pid_t pid) {
    const char *state = get_process_state_string(pid);

    if (is_interactive_terminal()) {
        const char *color = ANSI_RESET;
        if (strcmp(state, "RUNNING") == 0) color = ANSI_GREEN;
        else if (strcmp(state, "STOPPED") == 0) color = ANSI_YELLOW;
        else if (strcmp(state, "TERMINATED") == 0) color = ANSI_RED;
        else color = ANSI_MAGENTA;

        printf("\n" ANSI_BOLD "PID       :" ANSI_RESET " " ANSI_CYAN "%d" ANSI_RESET "\n", pid);
        printf(ANSI_BOLD "Process   :" ANSI_RESET " worker\n");
        printf(ANSI_BOLD "Status    :" ANSI_RESET " %s%s" ANSI_RESET "\n\n", color, state);
    } else {
        /* Che do chuan khong mau danh cho automated test grep */
        printf("\nPID       : %d\n", pid);
        printf("Process   : worker\n");
        printf("Status    : %s\n\n", state);
    }
}

/**
 * @brief Hien thi bang chon cac chuc nang quan ly tien trinh.
 */
void show_menu(void) {
    if (is_interactive_terminal()) {
        printf(ANSI_CYAN "----------------------------------------" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "1." ANSI_RESET " Suspend Process   (" ANSI_YELLOW "SIGSTOP" ANSI_RESET ")\n");
        printf("  " ANSI_BOLD "2." ANSI_RESET " Resume Process    (" ANSI_GREEN "SIGCONT" ANSI_RESET ")\n");
        printf("  " ANSI_BOLD "3." ANSI_RESET " Show Process Status\n");
        printf("  " ANSI_BOLD "4." ANSI_RESET " Terminate Process (" ANSI_RED "SIGTERM" ANSI_RESET ")\n");
        printf("  " ANSI_BOLD "5." ANSI_RESET " Exit\n");
        printf(ANSI_CYAN "----------------------------------------" ANSI_RESET "\n");
        printf(ANSI_BOLD "Enter choice: " ANSI_RESET);
    } else {
        printf("----------------------------------------\n");
        printf("1. Suspend Process\n");
        printf("2. Resume Process\n");
        printf("3. Show Process Status\n");
        printf("4. Terminate Process\n");
        printf("5. Exit\n");
        printf("----------------------------------------\n");
        printf("Enter choice: ");
    }
    fflush(stdout);
}

/**
 * @brief Hien thi chi tiet trang thai tien trinh khi chon menu 3.
 */
void show_status(pid_t pid) {
    const char *state = get_process_state_string(pid);

    if (is_interactive_terminal()) {
        const char *color = ANSI_RESET;
        if (strcmp(state, "RUNNING") == 0) color = ANSI_GREEN;
        else if (strcmp(state, "STOPPED") == 0) color = ANSI_YELLOW;
        else if (strcmp(state, "TERMINATED") == 0) color = ANSI_RED;
        else color = ANSI_MAGENTA;

        printf("\n" ANSI_BOLD "[Process Status Monitor]" ANSI_RESET "\n");
        printf(ANSI_BOLD "PID       :" ANSI_RESET " " ANSI_CYAN "%d" ANSI_RESET "\n", pid);
        printf(ANSI_BOLD "Process   :" ANSI_RESET " worker\n");
        printf(ANSI_BOLD "Status    :" ANSI_RESET " %s%s" ANSI_RESET "\n", color, state);
        printf(ANSI_CYAN "----------------------------------------" ANSI_RESET "\n");
    } else {
        printf("\n[Process Status Monitor]\n");
        printf("PID       : %d\n", pid);
        printf("Process   : worker\n");
        printf("Status    : %s\n", state);
        printf("----------------------------------------\n");
    }
}

/**
 * @brief Doc lua chon cua nguoi dung mot cach an toan va chong loi input.
 * 
 * - Dung fgets() thay vi scanf("%d") de tranh tran bo dem va tranh lap vo tan khi nhap ky tu chu.
 * - Dung strtol() de kiem tra tinh toan ven: neu co ky tu la khong phai so -> tra ve -1 (invalid).
 * - Neu phat hien EOF (End of file khi chay pipe) -> tu dong tra ve 5 de thoat an toan.
 */
int get_user_choice(void) {
    char buffer[64];

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        /* Dat EOF (vi du khi piped input ket thuc trong automated tests) */
        return 5;
    }

    /* Bo qua khoang trang o dau dong */
    char *start = buffer;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '\0') {
        return -1; /* Nguoi dung bam Enter hoac chi nhap khoang trang */
    }

    /* Chuyen chuoi sang so nguyen */
    char *endptr = NULL;
    long val = strtol(start, &endptr, 10);

    /* Bo qua khoang trang o cuoi chuoi neu co */
    while (endptr && isspace((unsigned char)*endptr)) {
        endptr++;
    }

    /* Neu con tro ket thuc chua cham '\0', nghia la co ky tu la (vd: 1abc, 2x) */
    if (endptr && *endptr != '\0') {
        return -1; /* Input khong hop le */
    }

    return (int)val;
}
