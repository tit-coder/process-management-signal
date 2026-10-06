#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

/**
 * @brief Creates a worker child process using fork().
 * 
 * The worker process executes a periodic running loop with sleep.
 * 
 * @return pid_t The child process PID in the parent process,
 *               or -1 if fork() fails.
 */
pid_t create_process(void);

/**
 * @brief Checks if a process with given PID is currently alive.
 * 
 * @param pid Process ID to check.
 * @return int 1 if alive, 0 if terminated or does not exist.
 */
int is_process_alive(pid_t pid);

/**
 * @brief Waits for a process to terminate to prevent zombie processes.
 * 
 * @param pid Process ID to wait for.
 * @return int Exit status of the child, or -1 on error.
 */
int wait_for_process(pid_t pid);

#endif /* PROCESS_MANAGER_H */
