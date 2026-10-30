#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>

/**
 * @brief Displays the terminal interactive menu for process management.
 */
void show_menu(void);

/**
 * @brief Displays the header information and basic status of the process.
 * 
 * @param pid Process ID of the worker process.
 */
void show_process_info(pid_t pid);

/**
 * @brief Displays the current detailed status of the process.
 * 
 * @param pid Process ID of the worker process.
 */
void show_status(pid_t pid);

/**
 * @brief Prompts user and safely reads the integer menu selection.
 * 
 * @return int The choice entered by the user, or -1 if invalid.
 */
int get_user_choice(void);

/**
 * @brief Helper to get the process state string (RUNNING, STOPPED, TERMINATED, UNKNOWN).
 * 
 * @param pid Process ID to query.
 * @return const char* String representing the state.
 */
const char* get_process_state_string(pid_t pid);

#endif /* MONITOR_H */
