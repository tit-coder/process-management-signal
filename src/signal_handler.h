#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <sys/types.h>

/*
 * Signal Control module
 *
 * Dieu khien Worker Process bang signal:
 *   suspend_process()   -> SIGSTOP (tam dung)
 *   resume_process()    -> SIGCONT (chay tiep)
 *   terminate_process() -> SIGTERM (ket thuc)
 *
 * Gia tri tra ve (ca 3 ham):
 *    0 : da gui signal thanh cong
 *   -1 : that bai (PID khong hop le, tien trinh khong ton tai,
 *        khong co quyen, ...). Loi da duoc in ra stderr.
 *
 * Luu y: terminate_process() chi GUI signal. Noi goi phai goi
 * wait_for_process() (process_manager) de thu hoi tien trinh, tranh Zombie.
 */

int suspend_process(pid_t pid);
int resume_process(pid_t pid);
int terminate_process(pid_t pid);

#endif /* SIGNAL_HANDLER_H */
