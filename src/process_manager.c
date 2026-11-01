#define _POSIX_C_SOURCE 200809L
#include "process_manager.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

pid_t create_process(void) {
    /* Do not duplicate the parent's buffered output in the child. */
    if (fflush(NULL) == EOF) {
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        int saved_errno = errno;
        perror("[ERROR] Failed to fork worker process");
        errno = saved_errno;
        return -1;
    }

    if (pid == 0) {
        /* main installs no handlers/mask. Preserve the team's signal policy. */
        printf("\n[Worker] Initialized successfully. Worker PID: %ld\n", (long)getpid());
        if (fflush(stdout) == EOF) {
            _exit(1);
        }

        unsigned long long counter = 1;
        while (1) {
            printf("Running... %llu\n", counter++);
            if (fflush(stdout) == EOF) {
                _exit(1);
            }
            /* Complete the interval even if a future handler interrupts it. */
            unsigned int remaining = 1;
            while ((remaining = sleep(remaining)) != 0) {
            }
        }
    }

    /* Parent Process */
    return pid;
}

int is_process_alive(pid_t pid) {
    if (pid <= 0) {
        errno = EINVAL;
        return 0;
    }

    /* EPERM proves existence, but still inspect state for zombies. */
    if (kill(pid, 0) == -1 && errno != EPERM) {
        if (errno == ESRCH) {
            return 0;
        }
        /* Existing callers use a boolean: unknown must not mean dead. */
        return 1;
    }

    /* Linux /proc avoids consuming the exit status needed by wait_for_process. */
    char path[64];
    snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* The process may have disappeared since kill(pid, 0). */
        if (errno == ENOENT || errno == ESRCH) {
            errno = ESRCH;
            return 0;
        }
        return 1; /* Unknown state; retain the inspection error. */
    }

    char line[256];
    int state = 0;
    errno = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "State:", 6) == 0) {
            char *p = line + 6;
            while (*p == ' ' || *p == '\t') p++;
            state = (unsigned char)*p;
            if (state == '\n') state = 0;
            break;
        }
    }
    int saved_errno = ferror(fp) ? (errno ? errno : EIO) : 0;
    if (fclose(fp) == EOF && saved_errno == 0) {
        saved_errno = errno;
    }
    errno = saved_errno;
    if (!state) {
        if (errno == 0) errno = EIO;
        return 1;
    }
    return state != 'Z' && state != 'X' && state != 'x';
}

int wait_for_process(pid_t pid) {
    if (pid <= 0) {
        errno = EINVAL;
        return -1;
    }

    int status;
    pid_t result;
    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        if (errno == ECHILD) {
            /* Process has already been reaped or does not exist as child */
            return 0;
        }
        return -1;
    }

    errno = 0;
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return WTERMSIG(status);
    }

    return 0;
}
