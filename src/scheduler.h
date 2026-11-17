/**
 * @file scheduler.h
 * @brief Header for Preemptive User-Space Round-Robin Scheduler using POSIX signals
 * @author Leader (tit-coder) - Linux System Programming Team
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>

#define DEFAULT_TIME_QUANTUM 2

/**
 * @brief Initialize the Round-Robin Scheduler with a time quantum.
 * 
 * @param quantum_seconds Time quantum in seconds (default: 2).
 * @return int 0 on success, -1 on failure.
 */
int scheduler_init(int quantum_seconds);

/**
 * @brief Start the preemptive Round-Robin scheduler.
 * 
 * Suspends all workers, activates the first, arms SIGALRM timer, and begins
 * periodic preemptive context-switching using SIGSTOP and SIGCONT.
 * 
 * @return int 0 on success, -1 on failure.
 */
int scheduler_start(void);

/**
 * @brief Stop the scheduler and restore manual control.
 * 
 * Disarms SIGALRM timer and resumes all workers.
 * 
 * @return int 0 on success, -1 on failure.
 */
int scheduler_stop(void);

/**
 * @brief Query if the scheduler is currently running.
 * 
 * @return int 1 if active, 0 if stopped.
 */
int scheduler_is_running(void);

/**
 * @brief Get the configured time quantum in seconds.
 * 
 * @return int Time quantum.
 */
int scheduler_get_quantum(void);

/**
 * @brief Set a new time quantum in seconds.
 * 
 * @param q Quantum in seconds.
 */
void scheduler_set_quantum(int q);

/**
 * @brief Get the PID of the worker currently allocated CPU by the scheduler.
 * 
 * @return pid_t Worker PID, or 0 if none.
 */
pid_t scheduler_get_current_pid(void);

/**
 * @brief Manually execute a single Round-Robin scheduling step (context switch).
 * Useful for deterministic testing.
 * 
 * @return int Index of newly scheduled worker, or -1 on error.
 */
int scheduler_step_tick(void);

#endif /* SCHEDULER_H */
