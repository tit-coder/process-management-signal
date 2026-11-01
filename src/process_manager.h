#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

/**
 * @brief Creates a worker child process using fork().
 * 
 * The parent returns immediately; the child never returns to the caller and
 * prints an increasing counter with sleep(1), flushing output even on a pipe.
 * Flushes all output streams before fork to avoid duplicate buffered output.
 * Call from the single-threaded manager, outside signal handlers.
 * Signal dispositions and mask are inherited unchanged. The current main has
 * no handlers/mask; if that changes, the Leader must ensure SIGTERM is neither
 * ignored nor blocked in the worker. No handlers are installed here.
 * 
 * @return pid_t The child process PID in the parent process,
 *               or -1 if flushing/fork fails, with errno describing the error.
 */
pid_t create_process(void);

/**
 * @brief Observes whether a PID has not terminated, without reaping it.
 *
 * Uses kill(pid, 0) and Linux /proc/<pid>/status. Stopped processes are alive;
 * zombies/dead processes are not. Works for observable PIDs, not only children.
 * EPERM from kill means existence, not death; /proc is still checked for zombies.
 * Inspection errors return 1 conservatively because existing callers require a
 * boolean, with errno set to the error (unknown state). A normal state read sets
 * errno to zero. Requires a mounted /proc in the same PID namespace on Ubuntu.
 * This snapshot cannot prevent exit or PID reuse before the next operation.
 * The caller must retain ownership of its worker PID and clear it after reaping.
 * 
 * @param pid Process ID to check.
 * @return int 1 if alive or state unknown; 0 if terminated, absent (ESRCH),
 *             or pid <= 0 (EINVAL). Never consumes a child's exit status.
 */
int is_process_alive(pid_t pid);

/**
 * @brief Blocks until the specified child terminates, then reaps it.
 *
 * Retries waitpid on EINTR. Never waits for a process group or arbitrary child.
 * Does not send signals: terminate/resume a stopped worker before calling this.
 * Do not call immediately after create_process on the interactive menu path.
 * Call even if is_process_alive reports a zombie. Only pass the owned child PID.
 * 
 * @param pid Process ID to wait for.
 * @return int Normal exit code, or terminating signal number (existing API
 *             convention), with errno = 0 after reaping. These values overlap;
 *             this API cannot distinguish an exit code from a signal number.
 *             ECHILD returns 0 with errno = ECHILD for compatibility (already
 *             reaped, not a child, or auto-reaped via SIGCHLD policy).
 *             Other failures return -1 with errno preserved; pid <= 0 sets
 *             EINVAL. No success is claimed for a new reap on ECHILD.
 */
int wait_for_process(pid_t pid);

#endif /* PROCESS_MANAGER_H */
