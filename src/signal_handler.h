/**
 * @file signal_handler.h
 * @brief Header for signal control and security validation module
 * @author TV3 - Member of Linux System Programming Team (tit-coder)
 */

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <sys/types.h>

/**
 * @brief Initialize parent signal handlers for clean teardown on SIGINT/SIGTERM.
 */
void sig_init_parent_handlers(void);

/**
 * @brief Suspend a specific worker process using SIGSTOP.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int sig_send_suspend(pid_t pid);

/**
 * @brief Resume a specific worker process using SIGCONT.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int sig_send_resume(pid_t pid);

/**
 * @brief Request termination of a specific worker process using SIGTERM.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int sig_send_terminate(pid_t pid);

/**
 * @brief Suspend ALL workers in the pool (Broadcast SIGSTOP via PGID).
 * 
 * @return int 0 on success, -1 on failure.
 */
int sig_suspend_all(void);

/**
 * @brief Resume ALL workers in the pool (Broadcast SIGCONT via PGID).
 * 
 * @return int 0 on success, -1 on failure.
 */
int sig_resume_all(void);

/**
 * @brief Append an audit log entry to logs/audit.log.
 * 
 * @param action Signal name or action (e.g. "SIGSTOP", "SIGCONT").
 * @param target_pid Target process ID.
 * @param status Result status string ("SUCCESS", "REJECTED", "ERROR").
 */
void sig_audit_log(const char *action, pid_t target_pid, const char *status);

#endif /* SIGNAL_HANDLER_H */
