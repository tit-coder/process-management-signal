#define _POSIX_C_SOURCE 200809L
#include "process_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

pid_t create_process(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("[ERROR] Failed to fork worker process");
        return -1;
    }

    if (pid == 0) {
        /* Worker Child Process */
        printf("\n[Worker] Initialized successfully. Worker PID: %d\n", getpid());
        fflush(stdout);

        int counter = 1;
        while (1) {
            printf("Running... %d\n", counter++);
            fflush(stdout);
            sleep(1);
        }

        /* Should not be reached */
        exit(EXIT_SUCCESS);
    }

    /* Parent Process */
    return pid;
}

int is_process_alive(pid_t pid) {
    if (pid <= 0) {
        return 0;
    }

    /* Check if child has exited and can be reaped */
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == pid) {
        /* Child already terminated and has been reaped */
        return 0;
    }

    /* Check existence via kill(pid, 0) */
    if (kill(pid, 0) == 0) {
        /* Also inspect /proc/<pid>/status to detect zombie state */
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *fp = fopen(path, "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "State:", 6) == 0) {
                    char *p = line + 6;
                    while (*p == ' ' || *p == '\t') p++;
                    if (*p == 'Z' || *p == 'X') {
                        fclose(fp);
                        return 0; /* Zombie or Dead */
                    }
                    break;
                }
            }
            fclose(fp);
        }
        return 1;
    }

    if (errno == ESRCH) {
        return 0; /* Process does not exist */
    } else if (errno == EPERM) {
        return 1; /* Process exists, no permission to send signal */
    }

    return 0;
}

int wait_for_process(pid_t pid) {
    if (pid <= 0) {
        return -1;
    }

    int status;
    pid_t result = waitpid(pid, &status, 0);
    if (result == -1) {
        if (errno == ECHILD) {
            /* Process has already been reaped or does not exist as child */
            return 0;
        }
        perror("[ERROR] waitpid failed");
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return WTERMSIG(status);
    }

    return 0;
}
