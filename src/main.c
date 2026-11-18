/**
 * @file main.c
 * @brief Coordinator module for Linux Process Management System
 * @author Leader (tit-coder) - Linux System Programming Team
 */

#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"
#include "signal_handler.h"
#include "monitor.h"
#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static void print_banner(void) {
    printf("\n=================================================================\n");
    printf("   LINUX PROCESS MANAGEMENT SYSTEM (C11 / POSIX.1-2008)          \n");
    printf("   Subject: System Programming - VKU Faculty of Computer Science \n");
    printf("   Team Leader: tit-coder | Repo: linux-process-control         \n");
    printf("   Architecture: Process Pool & Preemptive Round-Robin Scheduler \n");
    printf("=================================================================\n");
}

static void print_menu(void) {
    worker_pool_t *pool = pm_get_pool();
    int active_workers = 0;
    if (pool != NULL) {
        for (int i = 0; i < MAX_WORKERS; i++) {
            if (pool->workers[i].is_active && pool->workers[i].pid > 0) {
                active_workers++;
            }
        }
    }

    printf("\n----------------------- [ CONTROL MENU ] ------------------------\n");
    printf(" Workers: %d active | PGID: %d | Scheduler: [%s]\n",
           active_workers,
           (int)pm_get_pgid(),
           scheduler_is_running() ? "ACTIVE (2s)" : "OFF");
    printf("-----------------------------------------------------------------\n");
    printf("  -- MANUAL PROCESS CONTROL --\n");
    printf("  1. Suspend a specific Worker (Send SIGSTOP)\n");
    printf("  2. Resume a specific Worker  (Send SIGCONT)\n");
    printf("  3. Suspend ALL Workers       (Broadcast SIGSTOP via PGID)\n");
    printf("  4. Resume ALL Workers        (Broadcast SIGCONT via PGID)\n");
    printf("  5. Show Pool Status Dashboard(Real-time /proc telemetry)\n");
    printf("  6. Manage Worker Pool        (Spawn new / Terminate worker)\n");
    printf("  -- AUTO-SCHEDULER (ROUND-ROBIN) --\n");
    printf("  7. Start Auto-Scheduler      (Round-Robin 2s Quantum via SIGALRM)\n");
    printf("  8. Stop Auto-Scheduler       (Return to manual control)\n");
    printf("  -- SYSTEM --\n");
    printf("  9. Exit                      (Gracefully terminate all & quit)\n");
    printf("-----------------------------------------------------------------\n");
    printf("Enter your choice [1-9]: ");
    fflush(stdout);
}

/**
 * @brief Safely read an integer choice from user input.
 */
static int read_user_choice(void) {
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        // EOF encountered (e.g. piped input ended or Ctrl+D)
        return 9; // Exit
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (strlen(buffer) == 0) {
        return -1; // Empty line
    }

    char *endptr = NULL;
    long val = strtol(buffer, &endptr, 10);
    if (*endptr != '\0' || val < 1 || val > 9) {
        return -1; // Invalid integer
    }

    return (int)val;
}

/**
 * @brief Prompt user for a target PID or Worker ID with smart fallback.
 */
static pid_t prompt_target_pid(const char *action_name) {
    worker_pool_t *pool = pm_get_pool();
    if (!pool || pool->count == 0) {
        printf("[Warning] No workers currently active in pool.\n");
        return 0;
    }

    // Default target: first active worker
    pid_t default_pid = 0;
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pool->workers[i].is_active && pool->workers[i].pid > 0) {
            default_pid = pool->workers[i].pid;
            break;
        }
    }

    // If stdin is not an interactive terminal (e.g. piped in automated script), use default
    if (!isatty(STDIN_FILENO)) {
        return default_pid;
    }

    printf("Enter target Worker PID or ID to %s [Press Enter for default: PID %d]: ",
           action_name, (int)default_pid);
    fflush(stdout);

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return default_pid;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    if (strlen(buffer) == 0) {
        return default_pid;
    }

    char *endptr = NULL;
    long input_val = strtol(buffer, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "[Error] Invalid non-numeric input '%s'.\n", buffer);
        return -1;
    }

    // Check if input_val is a Worker ID (1 to 10)
    worker_node_t *w_by_id = pm_find_worker_by_id((int)input_val);
    if (w_by_id != NULL) {
        return w_by_id->pid;
    }

    return (pid_t)input_val;
}

/**
 * @brief Automated test dispatcher for CLI test suites.
 */
static int handle_cli_mode(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "--test-signal") == 0) {
        // Syntax: --test-signal <SIGSTOP|SIGCONT|SIGTERM> <PID>
        const char *sig_name = argv[2];
        char *endptr = NULL;
        long target_pid_l = (argc >= 4) ? strtol(argv[3], &endptr, 10) : -1;
        pid_t target_pid = (pid_t)target_pid_l;

        if (argc < 4 || *endptr != '\0') {
            fprintf(stderr, "[Error] Missing or invalid target PID: '%s'\n", (argc >= 4) ? argv[3] : "EMPTY");
            return 1;
        }

        pm_set_worker_pid(target_pid);

        if (strcmp(sig_name, "SIGSTOP") == 0) {
            return (sig_send_suspend(target_pid) == 0) ? 0 : 1;
        } else if (strcmp(sig_name, "SIGCONT") == 0) {
            return (sig_send_resume(target_pid) == 0) ? 0 : 1;
        } else if (strcmp(sig_name, "SIGTERM") == 0) {
            return (sig_send_terminate(target_pid) == 0) ? 0 : 1;
        } else {
            fprintf(stderr, "[Error] Unknown signal option: %s\n", sig_name);
            return 1;
        }
    } else if (argc >= 3 && strcmp(argv[1], "--test-status") == 0) {
        char *endptr = NULL;
        long target_pid_l = strtol(argv[2], &endptr, 10);
        if (*endptr != '\0' || target_pid_l <= 0) {
            fprintf(stderr, "[Error] Invalid PID '%s' for status check.\n", argv[2]);
            return 1;
        }
        return (monitor_show_process_status((pid_t)target_pid_l) == 0) ? 0 : 1;
    } else if (argc >= 2 && strcmp(argv[1], "--test-invalid-pid") == 0) {
        const char *pid_str = (argc >= 3) ? argv[2] : "-999";
        char *endptr = NULL;
        long pid_l = strtol(pid_str, &endptr, 10);
        if (*endptr != '\0' || pid_l <= 0) {
            fprintf(stderr, "[Validation] Successfully rejected malformed/non-positive PID: '%s'\n", pid_str);
            return 0;
        }
        pm_set_worker_pid(12345);
        if (sig_send_suspend((pid_t)pid_l) != 0) {
            fprintf(stderr, "[Validation] Correctly denied operation on unmanaged/invalid PID: %ld\n", pid_l);
            return 0;
        }
        return 1;
    } else if (argc >= 2 && strcmp(argv[1], "--test-pool") == 0) {
        printf("[Test Mode] Verifying Multi-Worker Process Pool Operations...\n");
        if (pm_pool_init(3) != 0) return 1;
        worker_pool_t *p = pm_get_pool();
        if (!p || p->count != 3) return 1;

        struct timespec pause_100ms = { .tv_sec = 0, .tv_nsec = 100000000L };
        printf("[Test Mode] Testing Suspend All...\n");
        sig_suspend_all();
        nanosleep(&pause_100ms, NULL);

        printf("[Test Mode] Testing Resume All...\n");
        sig_resume_all();
        nanosleep(&pause_100ms, NULL);

        printf("[Test Mode] Testing Dynamic Worker Spawn...\n");
        pid_t new_w = pm_pool_spawn_worker();
        if (new_w <= 0) return 1;

        printf("[Test Mode] Testing Worker Termination...\n");
        pm_pool_terminate_worker(new_w);

        printf("[Test Mode] Testing Pool Teardown...\n");
        pm_pool_cleanup_all();
        printf("[Test Mode] Process Pool Verification PASSED.\n");
        return 0;
    } else if (argc >= 2 && strcmp(argv[1], "--test-scheduler") == 0) {
        printf("[Test Mode] Verifying Preemptive Round-Robin Scheduler...\n");
        if (pm_pool_init(3) != 0) return 1;
        scheduler_init(1); // 1-second quantum for fast testing

        printf("[Test Mode] Executing Round-Robin step 1...\n");
        int w1 = scheduler_step_tick();
        if (w1 < 0) return 1;

        printf("[Test Mode] Executing Round-Robin step 2...\n");
        int w2 = scheduler_step_tick();
        if (w2 < 0 || w2 == w1) return 1;

        printf("[Test Mode] Executing Round-Robin step 3...\n");
        int w3 = scheduler_step_tick();
        if (w3 < 0) return 1;

        pm_pool_cleanup_all();
        printf("[Test Mode] Preemptive Scheduler Verification PASSED.\n");
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    // Check if invoked in CLI automated test mode
    int cli_ret = handle_cli_mode(argc, argv);
    if (cli_ret != -1) {
        return cli_ret;
    }

    // Set up signal handlers for parent process to avoid leaving orphan/zombie processes
    sig_init_parent_handlers();

    print_banner();

    // Initialize multi-worker pool with 3 initial workers
    printf("\n[Initiating] Initializing Multi-Worker Process Pool via fork()...\n");
    if (pm_pool_init(DEFAULT_INITIAL_WORKERS) != 0) {
        fprintf(stderr, "[Error] Failed to initialize worker pool. Exiting.\n");
        return EXIT_FAILURE;
    }

    scheduler_init(DEFAULT_TIME_QUANTUM);

    int running = 1;
    while (running) {
        pm_reap_zombies();

        print_menu();
        int choice = read_user_choice();

        switch (choice) {
            case 1: { // Suspend specific worker
                printf("\n--- [Action 1: Suspend Specific Worker] ---\n");
                pid_t target = prompt_target_pid("SUSPEND");
                if (target > 0) {
                    sig_send_suspend(target);
                }
                break;
            }
            case 2: { // Resume specific worker
                printf("\n--- [Action 2: Resume Specific Worker] ---\n");
                pid_t target = prompt_target_pid("RESUME");
                if (target > 0) {
                    sig_send_resume(target);
                }
                break;
            }
            case 3: { // Suspend all workers
                printf("\n--- [Action 3: Suspend ALL Workers] ---\n");
                sig_suspend_all();
                break;
            }
            case 4: { // Resume all workers
                printf("\n--- [Action 4: Resume ALL Workers] ---\n");
                sig_resume_all();
                break;
            }
            case 5: { // Show Pool Status Dashboard
                printf("\n--- [Action 5: Process Pool Dashboard] ---\n");
                monitor_show_pool_dashboard();
                break;
            }
            case 6: { // Manage Pool
                printf("\n--- [Action 6: Manage Worker Pool] ---\n");
                printf("  1. Spawn a new Worker child\n");
                printf("  2. Terminate a Worker child by PID\n");
                printf("Select option [1-2]: ");
                fflush(stdout);

                char sub_buf[32];
                if (fgets(sub_buf, sizeof(sub_buf), stdin)) {
                    int sub_c = atoi(sub_buf);
                    if (sub_c == 1) {
                        pm_pool_spawn_worker();
                    } else if (sub_c == 2) {
                        pid_t kill_pid = prompt_target_pid("TERMINATE");
                        if (kill_pid > 0) {
                            pm_pool_terminate_worker(kill_pid);
                        }
                    } else {
                        printf("[Invalid Option] Cancelled.\n");
                    }
                }
                break;
            }
            case 7: { // Start Auto-Scheduler
                printf("\n--- [Action 7: Start Auto-Scheduler] ---\n");
                scheduler_start();
                break;
            }
            case 8: { // Stop Auto-Scheduler
                printf("\n--- [Action 8: Stop Auto-Scheduler] ---\n");
                scheduler_stop();
                break;
            }
            case 9: { // Exit
                printf("\n--- [Action 9: Graceful Exit] ---\n");
                if (scheduler_is_running()) {
                    scheduler_stop();
                }
                pm_pool_cleanup_all();
                running = 0;
                printf("[Shutdown] Linux Process Management System exited cleanly.\n");
                break;
            }
            default:
                printf("\n[Invalid Input] Please enter a valid option number between 1 and 9.\n");
                break;
        }

        struct timespec pause_ts = { .tv_sec = 0, .tv_nsec = 200000000L }; // 200ms
        nanosleep(&pause_ts, NULL);
    }

    return EXIT_SUCCESS;
}
