#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>

/**
 * @brief Hien thi menu tuong tac tren terminal cho quan ly tien trinh.
 */
void show_menu(void);

/**
 * @brief Hien thi thong tin co ban va trang thai tien trinh worker.
 * 
 * @param pid PID cua worker process.
 */
void show_process_info(pid_t pid);

/**
 * @brief Hien thi chi tiet trang thai hien tai cua tien trinh.
 * 
 * @param pid PID cua worker process.
 */
void show_status(pid_t pid);

/**
 * @brief Nhap va kiem tra an toan lua chon so nguyen tu nguoi dung.
 * 
 * @return int Lua chon cua nguoi dung, hoac -1 neu khong hop le.
 */
int get_user_choice(void);

/**
 * @brief Ham bo tro lay chuoi trang thai tien trinh tu /proc (RUNNING, STOPPED, TERMINATED, UNKNOWN).
 * 
 * @param pid PID cua tien trinh can tra cuu.
 * @return const char* Chuoi ky tu bieu dien trang thai.
 */
const char* get_process_state_string(pid_t pid);

#endif /* MONITOR_H */

