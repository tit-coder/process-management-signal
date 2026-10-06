#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "process_manager.h"
#include "signal_handler.h"
#include "monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    /* Create the worker process */
    pid_t worker_pid = create_process();
    if (worker_pid < 0) {
        fprintf(stderr, "[FATAL] Failed to create worker process. Exiting.\n");
        return EXIT_FAILURE;
    }

    /* Brief pause to allow the worker to print initial status */
    usleep(100000);

    int running = 1;
    while (running) {
        printf("\n========================================\n");
        printf("       PROCESS MANAGEMENT SYSTEM        \n");
        printf("========================================\n");
        show_process_info(worker_pid);
        show_menu();

        int choice = get_user_choice();

        switch (choice) {
            case 1:
                /* Suspend Process */
                if (!is_process_alive(worker_pid)) {
                    printf("[INFO] Process %d is already terminated. Cannot suspend.\n", worker_pid);
                } else {
                    suspend_process(worker_pid);
                }
                break;

            case 2:
                /* Resume Process */
                if (!is_process_alive(worker_pid)) {
                    printf("[INFO] Process %d is already terminated. Cannot resume.\n", worker_pid);
                } else {
                    resume_process(worker_pid);
                }
                break;

            case 3:
                /* Show Process Status */
                show_status(worker_pid);
                break;

            case 4:
                /* Terminate Process */
                if (!is_process_alive(worker_pid)) {
                    printf("[INFO] Process %d is already terminated.\n", worker_pid);
                } else {
                    terminate_process(worker_pid);
                    wait_for_process(worker_pid);
                }
                break;

            case 5:
                /* Exit */
                printf("[INFO] Exiting Process Management System...\n");
                if (is_process_alive(worker_pid)) {
                    printf("[INFO] Terminating active worker process %d before exit...\n", worker_pid);
                    terminate_process(worker_pid);
                    wait_for_process(worker_pid);
                }
                running = 0;
                break;

            default:
                printf("[ERROR] Invalid choice. Please enter a number between 1 and 5.\n");
                break;
        }

        /* Short delay to keep output ordered */
        usleep(150000);
    }

    printf("[INFO] Goodbye.\n");
    return EXIT_SUCCESS;
}
