/**
 * @file scheduler.c
 * @brief Implementation of Preemptive User-Space Round-Robin Scheduler using POSIX signals
 * @author Leader (tit-coder) - Linux System Programming Team
 */

#define _POSIX_C_SOURCE 200809L

#include "scheduler.h"
#include "process_manager.h"
#include "signal_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

static volatile sig_atomic_t g_scheduler_active = 0;
static int g_current_index = -1;
static int g_time_quantum = DEFAULT_TIME_QUANTUM;

/**
 * @brief Convert integer to ASCII string safely without malloc (Async-Signal-Safe).
 */
static int safe_itoa(int val, char *buf, int max_len) {
    if (max_len <= 1) return 0;
    char temp[32];
    int p = 0;
    int is_neg = 0;

    if (val < 0) {
        is_neg = 1;
        val = -val;
    }
    if (val == 0) {
        temp[p++] = '0';
    } else {
        while (val > 0 && p < 30) {
            temp[p++] = (char)('0' + (val % 10));
            val /= 10;
        }
    }
    if (is_neg && p < 31) {
        temp[p++] = '-';
    }

    int out_len = 0;
    while (p > 0 && out_len < max_len - 1) {
        buf[out_len++] = temp[--p];
    }
    buf[out_len] = '\0';
    return out_len;
}

int scheduler_step_tick(void) {
    worker_pool_t *pool = pm_get_pool();
    if (!pool || pool->count == 0) {
        return -1;
    }

    // 1. Preempt current running worker via SIGSTOP
    if (g_current_index >= 0 && g_current_index < MAX_WORKERS) {
        pid_t old_pid = pool->workers[g_current_index].pid;
        if (old_pid > 0 && pool->workers[g_current_index].is_active) {
            kill(old_pid, SIGSTOP);
            sig_audit_log("SCHED_PREEMPT", old_pid, "SIGSTOP");
        }
    }

    // 2. Select next active worker in Round-Robin order
    int start = (g_current_index >= 0) ? (g_current_index + 1) : 0;
    int next_idx = -1;

    for (int i = 0; i < MAX_WORKERS; i++) {
        int idx = (start + i) % MAX_WORKERS;
        if (pool->workers[idx].is_active && pool->workers[idx].pid > 0) {
            next_idx = idx;
            break;
        }
    }

    if (next_idx == -1) {
        // No active worker found
        g_current_index = -1;
        return -1;
    }

    g_current_index = next_idx;
    pid_t new_pid = pool->workers[next_idx].pid;

    // 3. Dispatch execution time slice to next worker via SIGCONT
    kill(new_pid, SIGCONT);
    sig_audit_log("SCHED_DISPATCH", new_pid, "SIGCONT");

    // 4. Emit safe notification directly via write()
    char buf[128];
    char num_buf[16];
    int len = 0;

    const char p1[] = "\n[Scheduler SIGALRM] Time Quantum expired! Switched to Worker ";
    memcpy(buf + len, p1, sizeof(p1) - 1);
    len += (int)(sizeof(p1) - 1);

    int id_len = safe_itoa(pool->workers[next_idx].id, num_buf, sizeof(num_buf));
    memcpy(buf + len, num_buf, id_len);
    len += id_len;

    const char p2[] = " (PID ";
    memcpy(buf + len, p2, sizeof(p2) - 1);
    len += (int)(sizeof(p2) - 1);

    int pid_len = safe_itoa((int)new_pid, num_buf, sizeof(num_buf));
    memcpy(buf + len, num_buf, pid_len);
    len += pid_len;

    const char p3[] = ").\n";
    memcpy(buf + len, p3, sizeof(p3) - 1);
    len += (int)(sizeof(p3) - 1);

    ssize_t written = write(STDOUT_FILENO, buf, len);
    (void)written;

    return next_idx;
}

/**
 * @brief Signal handler for SIGALRM timer interrupts.
 */
static void scheduler_alrm_handler(int sig) {
    (void)sig;
    if (g_scheduler_active) {
        scheduler_step_tick();
    }
}

int scheduler_init(int quantum_seconds) {
    g_time_quantum = (quantum_seconds > 0) ? quantum_seconds : DEFAULT_TIME_QUANTUM;
    g_current_index = -1;
    g_scheduler_active = 0;
    return 0;
}

int scheduler_start(void) {
    worker_pool_t *pool = pm_get_pool();
    if (!pool || pool->count == 0) {
        printf("[Scheduler] Error: Cannot start scheduler on empty worker pool.\n");
        return -1;
    }

    if (g_scheduler_active) {
        printf("[Scheduler] Scheduler is already actively running.\n");
        return 0;
    }

    printf("\n>>> [Scheduler] Starting User-Space Preemptive Round-Robin Engine (Quantum: %ds) <<<\n",
           g_time_quantum);

    // Initial state: Suspend all workers first
    sig_suspend_all();

    // Trigger initial dispatch
    scheduler_step_tick();

    // Configure SIGALRM handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = scheduler_alrm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("[Scheduler] sigaction(SIGALRM) failed");
        return -1;
    }

    // Arm periodic real-time interval timer
    struct itimerval itv;
    itv.it_interval.tv_sec = g_time_quantum;
    itv.it_interval.tv_usec = 0;
    itv.it_value = itv.it_interval;

    if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
        perror("[Scheduler] setitimer failed");
        return -1;
    }

    g_scheduler_active = 1;
    printf("[Scheduler] Periodic timer armed. Workers will now preemptively alternate every %ds.\n",
           g_time_quantum);
    return 0;
}

int scheduler_stop(void) {
    if (!g_scheduler_active) {
        printf("[Scheduler] Scheduler is not currently running.\n");
        return 0;
    }

    // Disarm timer
    struct itimerval itv;
    memset(&itv, 0, sizeof(itv));
    setitimer(ITIMER_REAL, &itv, NULL);

    // Reset SIGALRM handler
    signal(SIGALRM, SIG_DFL);

    g_scheduler_active = 0;
    g_current_index = -1;

    printf("[Scheduler] Auto-Scheduler stopped. Resuming all workers to manual control mode.\n");
    sig_resume_all();
    return 0;
}

int scheduler_is_running(void) {
    return g_scheduler_active;
}

int scheduler_get_quantum(void) {
    return g_time_quantum;
}

void scheduler_set_quantum(int q) {
    if (q > 0) {
        g_time_quantum = q;
        if (g_scheduler_active) {
            struct itimerval itv;
            itv.it_interval.tv_sec = g_time_quantum;
            itv.it_interval.tv_usec = 0;
            itv.it_value = itv.it_interval;
            setitimer(ITIMER_REAL, &itv, NULL);
        }
    }
}

pid_t scheduler_get_current_pid(void) {
    worker_pool_t *pool = pm_get_pool();
    if (!pool || g_current_index < 0 || g_current_index >= MAX_WORKERS) {
        return 0;
    }
    return pool->workers[g_current_index].pid;
}
