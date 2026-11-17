/**
 * @file signal_handler.c
 * @brief Implementation of signal dispatching, safety validation, and audit logging
 * @author TV3 - Member of Linux System Programming Team (tit-coder)
 */

#define _POSIX_C_SOURCE 200809L

#include "signal_handler.h"
#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>

/**
 * @brief Signal handler for parent termination signals (SIGINT, SIGTERM).
 */
static void parent_termination_handler(int sig) {
    (void)sig;
    const char msg[] = "\n[Signal Controller] Parent caught interrupt signal. Cleaning up all workers...\n";
    ssize_t written = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    (void)written;
    pm_pool_cleanup_all();
    _exit(EXIT_SUCCESS);
}

void sig_init_parent_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = parent_termination_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[Signal Controller] sigaction(SIGINT) failed");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("[Signal Controller] sigaction(SIGTERM) failed");
    }
}

void sig_audit_log(const char *action, pid_t target_pid, const char *status) {
    FILE *fp = fopen("logs/audit.log", "a");
    if (!fp) return;

    time_t now = time(NULL);
    char time_str[32];
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

    fprintf(fp, "[%s] [ACTION: %-12s] Target PID: %-7d | Result: %s\n",
            time_str, action, target_pid, status);
    fclose(fp);
}

/**
 * @brief Validate target PID before signal transmission to ensure system safety.
 */
static int validate_target_pid(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[Signal Controller] Error: Invalid PID %d (PID must be positive).\n", pid);
        sig_audit_log("VALIDATE", pid, "REJECTED_INVALID_PID");
        return -1;
    }

    if (pid == 1) {
        fprintf(stderr, "[Signal Controller] Security Violation: Target PID 1 (system init/systemd) is protected! Signal rejected!\n");
        sig_audit_log("VALIDATE", pid, "SECURITY_VIOLATION_PID_1");
        return -1;
    }

    if (!pm_is_pid_in_pool(pid)) {
        fprintf(stderr, "[Signal Controller] Security Violation: Target PID %d is not managed by this system. Signal rejected!\n",
                pid);
        sig_audit_log("VALIDATE", pid, "SECURITY_VIOLATION_UNMANAGED_PID");
        return -1;
    }

    if (!pm_is_worker_alive(pid)) {
        fprintf(stderr, "[Signal Controller] Error: Process %d is no longer alive or does not exist.\n", pid);
        sig_audit_log("VALIDATE", pid, "REJECTED_NOT_ALIVE");
        return -1;
    }

    return 0;
}

int sig_send_suspend(pid_t pid) {
    if (validate_target_pid(pid) != 0) {
        return -1;
    }

    printf("[Signal Controller] Dispatching SIGSTOP to PID %d (Suspend/Pause)...\n", pid);
    if (kill(pid, SIGSTOP) == -1) {
        perror("[Signal Controller] Failed to send SIGSTOP");
        sig_audit_log("SIGSTOP", pid, "FAILED_KILL");
        return -1;
    }

    sig_audit_log("SIGSTOP", pid, "SUCCESS");
    printf("[Signal Controller] Success: SIGSTOP sent to PID %d. Process is now SUSPENDED (State: T).\n", pid);
    return 0;
}

int sig_send_resume(pid_t pid) {
    if (validate_target_pid(pid) != 0) {
        return -1;
    }

    printf("[Signal Controller] Dispatching SIGCONT to PID %d (Resume/Continue)...\n", pid);
    if (kill(pid, SIGCONT) == -1) {
        perror("[Signal Controller] Failed to send SIGCONT");
        sig_audit_log("SIGCONT", pid, "FAILED_KILL");
        return -1;
    }

    sig_audit_log("SIGCONT", pid, "SUCCESS");
    printf("[Signal Controller] Success: SIGCONT sent to PID %d. Process is now RESUMED (State: S/R).\n", pid);
    return 0;
}

int sig_send_terminate(pid_t pid) {
    if (validate_target_pid(pid) != 0) {
        return -1;
    }

    printf("[Signal Controller] Dispatching SIGTERM to PID %d (Graceful Termination Request)...\n", pid);
    if (kill(pid, SIGTERM) == -1) {
        perror("[Signal Controller] Failed to send SIGTERM");
        sig_audit_log("SIGTERM", pid, "FAILED_KILL");
        return -1;
    }

    sig_audit_log("SIGTERM", pid, "SUCCESS");
    printf("[Signal Controller] Success: SIGTERM sent to PID %d.\n", pid);
    return 0;
}

int sig_suspend_all(void) {
    worker_pool_t *pool = pm_get_pool();
    if (!pool || pool->count == 0) {
        printf("[Signal Controller] Worker pool is empty. Nothing to suspend.\n");
        return 0;
    }

    pid_t pgid = pm_get_pgid();
    printf("[Signal Controller] Broadcasting SIGSTOP to entire Process Group (PGID: %d)...\n", pgid);

    int count = 0;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pool->workers[i].is_active && pool->workers[i].pid > 0) {
            kill(pool->workers[i].pid, SIGSTOP);
            sig_audit_log("SIGSTOP_ALL", pool->workers[i].pid, "SUCCESS");
            count++;
        }
    }

    printf("[Signal Controller] Successfully suspended %d workers in pool.\n", count);
    return 0;
}

int sig_resume_all(void) {
    worker_pool_t *pool = pm_get_pool();
    if (!pool || pool->count == 0) {
        printf("[Signal Controller] Worker pool is empty. Nothing to resume.\n");
        return 0;
    }

    pid_t pgid = pm_get_pgid();
    printf("[Signal Controller] Broadcasting SIGCONT to entire Process Group (PGID: %d)...\n", pgid);

    int count = 0;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pool->workers[i].is_active && pool->workers[i].pid > 0) {
            kill(pool->workers[i].pid, SIGCONT);
            sig_audit_log("SIGCONT_ALL", pool->workers[i].pid, "SUCCESS");
            count++;
        }
    }

    printf("[Signal Controller] Successfully resumed %d workers in pool.\n", count);
    return 0;
}
