/**
 * @file process_manager.h
 * @brief Header for process management and process pool module
 * @author TV2 - Member of Linux System Programming Team (tit-coder)
 */

#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#define MAX_WORKERS 10
#define DEFAULT_INITIAL_WORKERS 3

/**
 * @brief Data structure representing an individual worker process in the pool.
 */
typedef struct {
    int id;                           // Logical Worker ID (1, 2, 3...)
    pid_t pid;                        // Operating System Process ID
    volatile unsigned long counter;   // Atomic heartbeat execution counter (shared memory)
    time_t start_time;                // Spawn timestamp
    volatile sig_atomic_t is_active;  // 1 if active, 0 if terminated
    char name[32];                    // Descriptive label, e.g. "worker-1"
} worker_node_t;

/**
 * @brief Process Pool container managing multiple worker children.
 */
typedef struct {
    worker_node_t workers[MAX_WORKERS]; // Fixed-size worker array in shared memory
    int count;                          // Total number of registered workers
    pid_t pgid;                         // Common Process Group ID for broadcast signals
} worker_pool_t;

/**
 * @brief Initialize the shared memory process pool and spawn initial workers.
 * 
 * @param initial_count Number of workers to spawn immediately (e.g., 3).
 * @return int 0 on success, -1 on failure.
 */
int pm_pool_init(int initial_count);

/**
 * @brief Dynamically spawn a single new worker child and add it to the pool.
 * 
 * @return pid_t PID of the newly spawned worker, or -1 on error.
 */
pid_t pm_pool_spawn_worker(void);

/**
 * @brief Terminate and reap a specific worker process in the pool.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int pm_pool_terminate_worker(pid_t pid);

/**
 * @brief Gracefully terminate and reap ALL active workers in the pool.
 */
void pm_pool_cleanup_all(void);

/**
 * @brief Get a pointer to the shared worker pool.
 * 
 * @return worker_pool_t* Pointer to the pool.
 */
worker_pool_t* pm_get_pool(void);

/**
 * @brief Get the Process Group ID (PGID) shared by the worker pool.
 * 
 * @return pid_t PGID, or 0 if uninitialized.
 */
pid_t pm_get_pgid(void);

/**
 * @brief Check if a PID belongs to an active worker in the pool.
 * 
 * @param pid PID to check.
 * @return int 1 if found in pool, 0 otherwise.
 */
int pm_is_pid_in_pool(pid_t pid);

/**
 * @brief Find a worker node by its PID.
 * 
 * @param pid Target PID.
 * @return worker_node_t* Pointer to worker node, or NULL if not found.
 */
worker_node_t* pm_find_worker_by_pid(pid_t pid);

/**
 * @brief Find a worker node by its logical ID (1, 2, 3...).
 * 
 * @param id Logical ID.
 * @return worker_node_t* Pointer to worker node, or NULL if not found.
 */
worker_node_t* pm_find_worker_by_id(int id);

/**
 * @brief Check if a worker process is still alive and responsive.
 * 
 * @param pid Target PID.
 * @return int 1 if alive, 0 if dead/terminated, -1 on invalid argument.
 */
int pm_is_worker_alive(pid_t pid);

/**
 * @brief Non-blocking sweep to reap terminated child processes (anti-zombie engine).
 */
void pm_reap_zombies(void);

/* ========================================================================= */
/* Legacy compatibility wrappers                                             */
/* ========================================================================= */
pid_t pm_spawn_worker(void);
pid_t pm_get_worker_pid(void);
void pm_set_worker_pid(pid_t pid);
int pm_terminate_and_cleanup(pid_t pid);
void pm_cleanup_on_exit(void);

#endif /* PROCESS_MANAGER_H */
