/**
 * @file process_manager.c
 * @brief Implementation of Multi-Worker Process Pool and Lifecycle Engine
 * @author TV2 - Member of Linux System Programming Team (tit-coder)
 */

#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

static worker_pool_t *g_pool = NULL;
static volatile sig_atomic_t g_worker_running = 1;

/**
 * @brief Signal handler inside the Worker child process.
 */
static void worker_sigterm_handler(int sig) {
    (void)sig;
    g_worker_running = 0;
}

/**
 * @brief Worker process execution loop.
 * 
 * Runs in background child context. Increments shared counter and writes
 * heartbeat to its private log file (logs/worker_<id>.log), avoiding stdout
 * text interleaving on parent's CLI menu.
 */
static void run_worker_loop(int worker_id, int pool_index) {
    // Install SIGTERM handler for graceful termination
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = worker_sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("[Worker] Failed to setup SIGTERM handler");
        exit(EXIT_FAILURE);
    }

    // Worker ignores SIGINT so parent coordinates teardown cleanly
    signal(SIGINT, SIG_IGN);

    pid_t pid = getpid();
    pid_t ppid = getppid();

    // Ensure logs directory exists
    mkdir("logs", 0755);

    char log_filename[64];
    snprintf(log_filename, sizeof(log_filename), "logs/worker_%d.log", worker_id);
    FILE *log_fp = fopen(log_filename, "w");

    if (log_fp) {
        fprintf(log_fp, "[Worker ID %d | PID %d] Started (Parent PID: %d).\n", worker_id, pid, ppid);
        fflush(log_fp);
    }

    while (g_worker_running) {
        sleep(1);
        if (!g_worker_running) {
            break;
        }

        // Increment shared counter in shared memory
        if (g_pool != NULL && pool_index >= 0 && pool_index < MAX_WORKERS) {
            g_pool->workers[pool_index].counter++;
        }

        if (log_fp) {
            time_t now = time(NULL);
            char time_str[32];
            struct tm tm_info;
            localtime_r(&now, &tm_info);
            strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_info);

            unsigned long cnt = (g_pool != NULL) ? g_pool->workers[pool_index].counter : 0;
            fprintf(log_fp, "[%s] [Worker %d | PID %d] Heartbeat tick: count = %lu\n",
                    time_str, worker_id, pid, cnt);
            fflush(log_fp);
        }
    }

    if (log_fp) {
        fprintf(log_fp, "[Worker ID %d | PID %d] Received SIGTERM. Exiting gracefully.\n", worker_id, pid);
        fclose(log_fp);
    }

    exit(EXIT_SUCCESS);
}

int pm_pool_init(int initial_count) {
    if (initial_count <= 0) {
        initial_count = DEFAULT_INITIAL_WORKERS;
    }
    if (initial_count > MAX_WORKERS) {
        initial_count = MAX_WORKERS;
    }

    mkdir("logs", 0755);

    // Allocate pool container in anonymous shared memory
    if (g_pool == NULL) {
        g_pool = mmap(NULL, sizeof(worker_pool_t),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (g_pool == MAP_FAILED) {
            perror("[Process Manager] mmap failed for worker pool");
            g_pool = NULL;
            return -1;
        }
        memset(g_pool, 0, sizeof(worker_pool_t));
        g_pool->pgid = 0;
        g_pool->count = 0;
    }

    printf("[Process Manager] Initializing Process Pool (Target: %d workers)...\n", initial_count);

    for (int i = 0; i < initial_count; i++) {
        pm_pool_spawn_worker();
    }

    return 0;
}

pid_t pm_pool_spawn_worker(void) {
    if (g_pool == NULL) {
        if (pm_pool_init(1) != 0) {
            return -1;
        }
    }

    // Find next available slot
    int slot = -1;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (!g_pool->workers[i].is_active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        fprintf(stderr, "[Process Manager] Cannot spawn worker: Pool capacity reached (%d/%d).\n",
                MAX_WORKERS, MAX_WORKERS);
        return -1;
    }

    int worker_id = slot + 1;

    // Flush user-space stdio buffers prior to fork() to avoid duplication in child
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("[Process Manager] fork() failed");
        return -1;
    }

    if (pid == 0) {
        // Child context: assign to process group
        if (g_pool->pgid == 0) {
            setpgid(0, 0);
            g_pool->pgid = getpid();
        } else {
            setpgid(0, g_pool->pgid);
        }

        run_worker_loop(worker_id, slot);
        _exit(EXIT_SUCCESS);
    }

    // Parent context: set process group for child
    if (g_pool->pgid == 0) {
        g_pool->pgid = pid;
    }
    setpgid(pid, g_pool->pgid);

    g_pool->workers[slot].id = worker_id;
    g_pool->workers[slot].pid = pid;
    g_pool->workers[slot].counter = 0;
    g_pool->workers[slot].start_time = time(NULL);
    g_pool->workers[slot].is_active = 1;
    snprintf(g_pool->workers[slot].name, sizeof(g_pool->workers[slot].name), "worker-%d", worker_id);

    g_pool->count++;

    printf("[Process Manager] Spawned worker process PID: %d (Worker %d, PGID: %d)\n",
           pid, worker_id, g_pool->pgid);
    return pid;
}

worker_pool_t* pm_get_pool(void) {
    return g_pool;
}

pid_t pm_get_pgid(void) {
    return (g_pool != NULL) ? g_pool->pgid : 0;
}

int pm_is_pid_in_pool(pid_t pid) {
    if (g_pool == NULL || pid <= 0) return 0;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (g_pool->workers[i].is_active && g_pool->workers[i].pid == pid) {
            return 1;
        }
    }
    return 0;
}

worker_node_t* pm_find_worker_by_pid(pid_t pid) {
    if (g_pool == NULL || pid <= 0) return NULL;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (g_pool->workers[i].is_active && g_pool->workers[i].pid == pid) {
            return &g_pool->workers[i];
        }
    }
    return NULL;
}

worker_node_t* pm_find_worker_by_id(int id) {
    if (g_pool == NULL || id <= 0) return NULL;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (g_pool->workers[i].is_active && g_pool->workers[i].id == id) {
            return &g_pool->workers[i];
        }
    }
    return NULL;
}

int pm_is_worker_alive(pid_t pid) {
    if (pid <= 0) {
        return -1;
    }

    int status = 0;
    pid_t res = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

    if (res == pid) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            worker_node_t *w = pm_find_worker_by_pid(pid);
            if (w) w->is_active = 0;
            return 0;
        }
        return 1;
    } else if (res == 0) {
        return 1;
    } else {
        if (errno == ECHILD) {
            if (kill(pid, 0) == 0) return 1;
            worker_node_t *w = pm_find_worker_by_pid(pid);
            if (w) w->is_active = 0;
            return 0;
        }
    }

    if (kill(pid, 0) == 0) return 1;

    worker_node_t *w = pm_find_worker_by_pid(pid);
    if (w) w->is_active = 0;
    return 0;
}

int pm_pool_terminate_worker(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[Process Manager] Invalid PID %d provided for termination.\n", pid);
        return -1;
    }

    worker_node_t *w = pm_find_worker_by_pid(pid);
    if (!w || !w->is_active) {
        printf("[Process Manager] Process %d is not in active pool or already terminated.\n", pid);
        return 0;
    }

    printf("[Process Manager] Sending SIGTERM to worker PID %d (Worker %d)...\n", pid, w->id);
    if (kill(pid, SIGTERM) == -1) {
        if (errno != ESRCH) {
            perror("[Process Manager] kill(SIGTERM) failed");
            return -1;
        }
    }
    // Wake up child if currently suspended (SIGSTOP) so it can execute SIGTERM handler
    kill(pid, SIGCONT);

    int status = 0;
    int reaped = 0;
    for (int i = 0; i < 10; ++i) {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            reaped = 1;
            if (WIFEXITED(status)) {
                printf("[Process Manager] Worker PID %d exited normally with status %d.\n",
                       pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[Process Manager] Worker PID %d killed by signal %d (%s).\n",
                       pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
            }
            break;
        } else if (r == -1 && errno == ECHILD) {
            reaped = 1;
            break;
        }
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L }; // 100ms
        nanosleep(&ts, NULL);
    }

    if (!reaped && kill(pid, 0) == 0) {
        printf("[Process Manager] Worker %d unresponsive to SIGTERM. Sending SIGKILL...\n", pid);
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        printf("[Process Manager] Worker PID %d forcibly killed and reaped.\n", pid);
    }

    w->is_active = 0;
    if (g_pool->count > 0) {
        g_pool->count--;
    }

    return 0;
}

void pm_pool_cleanup_all(void) {
    if (g_pool == NULL) return;

    printf("[Process Manager] Cleaning up all workers in pool (PGID: %d)...\n", g_pool->pgid);

    // Send SIGTERM to entire process group if available
    if (g_pool->pgid > 0) {
        kill(-g_pool->pgid, SIGTERM);
    }

    // Wait and reap each active worker
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (g_pool->workers[i].is_active && g_pool->workers[i].pid > 0) {
            pm_pool_terminate_worker(g_pool->workers[i].pid);
        }
    }

    // Final sweep
    pm_reap_zombies();
    printf("[Process Manager] All workers successfully cleaned up.\n");
}

void pm_reap_zombies(void) {
    int status = 0;
    pid_t reaped;
    while ((reaped = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[Process Manager] Reaped child process PID %d.\n", reaped);
        if (g_pool != NULL) {
            for (int i = 0; i < MAX_WORKERS; i++) {
                if (g_pool->workers[i].pid == reaped) {
                    g_pool->workers[i].is_active = 0;
                    if (g_pool->count > 0) g_pool->count--;
                    break;
                }
            }
        }
    }
}

/* ========================================================================= */
/* Legacy compatibility wrappers                                             */
/* ========================================================================= */
pid_t pm_spawn_worker(void) {
    return pm_pool_spawn_worker();
}

pid_t pm_get_worker_pid(void) {
    if (g_pool == NULL) return 0;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (g_pool->workers[i].is_active && g_pool->workers[i].pid > 0) {
            return g_pool->workers[i].pid;
        }
    }
    return 0;
}

void pm_set_worker_pid(pid_t pid) {
    if (g_pool == NULL) {
        pm_pool_init(1);
    }
    if (g_pool != NULL) {
        g_pool->workers[0].id = 1;
        g_pool->workers[0].pid = pid;
        g_pool->workers[0].is_active = 1;
        g_pool->count = 1;
    }
}

int pm_terminate_and_cleanup(pid_t pid) {
    return pm_pool_terminate_worker(pid);
}

void pm_cleanup_on_exit(void) {
    pm_pool_cleanup_all();
}
