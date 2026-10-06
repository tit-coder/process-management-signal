#define _POSIX_C_SOURCE 200809L
#include "signal_handler.h"
#include "process_manager.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int suspend_process(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[ERROR] Invalid PID: %d. Cannot suspend.\n", pid);
        return -1;
    }

    if (!is_process_alive(pid)) {
        fprintf(stderr, "[ERROR] Process %d is not alive or has terminated.\n", pid);
        return -1;
    }

    if (kill(pid, SIGSTOP) == -1) {
        fprintf(stderr, "[ERROR] Failed to suspend process %d (SIGSTOP): %s\n", pid, strerror(errno));
        return -1;
    }

    printf("[SUCCESS] Process %d suspended (SIGSTOP sent).\n", pid);
    return 0;
}

int resume_process(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[ERROR] Invalid PID: %d. Cannot resume.\n", pid);
        return -1;
    }

    if (!is_process_alive(pid)) {
        fprintf(stderr, "[ERROR] Process %d is not alive or has terminated.\n", pid);
        return -1;
    }

    if (kill(pid, SIGCONT) == -1) {
        fprintf(stderr, "[ERROR] Failed to resume process %d (SIGCONT): %s\n", pid, strerror(errno));
        return -1;
    }

    printf("[SUCCESS] Process %d resumed (SIGCONT sent).\n", pid);
    return 0;
}

int terminate_process(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[ERROR] Invalid PID: %d. Cannot terminate.\n", pid);
        return -1;
    }

    if (!is_process_alive(pid)) {
        fprintf(stderr, "[ERROR] Process %d is already terminated or does not exist.\n", pid);
        return -1;
    }

    if (kill(pid, SIGTERM) == -1) {
        fprintf(stderr, "[ERROR] Failed to terminate process %d (SIGTERM): %s\n", pid, strerror(errno));
        return -1;
    }

    printf("[SUCCESS] Process %d terminated (SIGTERM sent).\n", pid);
    return 0;
}
