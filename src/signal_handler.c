
#define _POSIX_C_SOURCE 200809L

#include "signal_handler.h"
#include "process_manager.h"   /* is_process_alive() */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Kiem tra PID hop le truoc khi gui signal.
 *  - pid <= 0 : kill(0, sig) gui cho CA NHOM tien trinh, kill(-1, sig) gui
 *    cho moi tien trinh co quyen -> tuyet doi khong cho phep.
 *  - pid == 1 : tien trinh init/systemd.
 *  - pid == getpid() : tranh tu dung / tu giet chuong trinh quan ly.
 *  - Tien trinh phai con ton tai.
 */
static int check_target(pid_t pid, const char *action)
{
    if (pid <= 1) {
        fprintf(stderr, "[ERROR] Invalid PID: %d. Cannot %s.\n", (int)pid, action);
        return -1;
    }
    if (pid == getpid()) {
        fprintf(stderr, "[ERROR] PID %d is the manager itself. Cannot %s.\n",
                (int)pid, action);
        return -1;
    }
    if (!is_process_alive(pid)) {
        fprintf(stderr, "[ERROR] Process %d is already terminated or does not exist.\n",
                (int)pid);
        return -1;
    }
    return 0;
}

/* Gui signal, in loi chi tiet theo errno neu that bai. */
static int send_signal(pid_t pid, int sig, const char *action, const char *signame)
{
    if (check_target(pid, action) != 0)
        return -1;

    if (kill(pid, sig) == -1) {
        int saved = errno;
        if (saved == EPERM)
            fprintf(stderr, "[ERROR] Permission denied: cannot send %s to process %d.\n",
                    signame, (int)pid);
        else if (saved == ESRCH)
            fprintf(stderr, "[ERROR] Process %d no longer exists.\n", (int)pid);
        else
            fprintf(stderr, "[ERROR] Failed to %s process %d (%s): %s\n",
                    action, (int)pid, signame, strerror(saved));
        errno = saved;
        return -1;
    }
    return 0;
}

int suspend_process(pid_t pid)
{
    if (send_signal(pid, SIGSTOP, "suspend", "SIGSTOP") != 0)
        return -1;
    printf("[SUCCESS] Process %d suspended (SIGSTOP sent).\n", (int)pid);
    return 0;
}

int resume_process(pid_t pid)
{
    if (send_signal(pid, SIGCONT, "resume", "SIGCONT") != 0)
        return -1;
    printf("[SUCCESS] Process %d resumed (SIGCONT sent).\n", (int)pid);
    return 0;
}

int terminate_process(pid_t pid)
{
    if (send_signal(pid, SIGTERM, "terminate", "SIGTERM") != 0)
        return -1;

    /*
     * Worker dang STOPPED se khong xu ly SIGTERM cho den khi duoc danh thuc
     * (SIGTERM nam cho trong hang doi). Gui them SIGCONT de worker luon ket
     * thuc duoc; voi worker dang chay thi SIGCONT khong co tac dung phu.
     */
    kill(pid, SIGCONT);

    printf("[SUCCESS] Process %d terminate requested (SIGTERM sent). "
           "Call wait_for_process() to reap it.\n", (int)pid);
    return 0;
}
