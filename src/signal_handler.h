#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <sys/types.h>

/**
 * @brief Suspends a target process by sending SIGSTOP signal.
 * 
 * @param pid Process ID of the process to suspend.
 * @return int 0 on success, -1 on failure.
 */
int suspend_process(pid_t pid);

/**
 * @brief Resumes a suspended target process by sending SIGCONT signal.
 * 
 * @param pid Process ID of the process to resume.
 * @return int 0 on success, -1 on failure.
 */
int resume_process(pid_t pid);

/**
 * @brief Terminates a target process by sending SIGTERM signal.
 * 
 * @param pid Process ID of the process to terminate.
 * @return int 0 on success, -1 on failure.
 */
int terminate_process(pid_t pid);

#endif /* SIGNAL_HANDLER_H */
