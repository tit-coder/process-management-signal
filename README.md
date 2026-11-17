# Linux Process Control System (C11 / POSIX)

[![Language](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform](https://img.shields.io/badge/Platform-Ubuntu%20Linux-orange.svg)](https://ubuntu.com/)
[![Standard](https://img.shields.io/badge/Standard-POSIX.1--2008-green.svg)](https://pubs.opengroup.org/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

Hệ thống quản lý và điều khiển vòng đời tiến trình trên Linux sử dụng tín hiệu POSIX (**POSIX Signals**) để Tạm dừng (**Suspend**), Tiếp tục (**Resume**), Giám sát (**Monitor via `/proc`**) và Kết thúc an toàn (**Graceful Termination**) – Đồ án môn học **Lập trình hệ thống (System Programming)**.

- **GitHub Repository**: [https://github.com/tit-coder/linux-process-control](https://github.com/tit-coder/linux-process-control)
- **Nhóm sinh viên thực hiện**: Nhóm 4 sinh viên VKU
- **Trưởng nhóm (Team Leader)**: `tit-coder` (Nguyễn Hoài Vy - 24IT318)
- **Kiến trúc nâng cấp**: **Multi-Worker Process Pool & User-Space Preemptive Round-Robin Scheduler**

---

## 1. Mục tiêu đồ án mở rộng

1. **Quản lý Bể tiến trình đa nhiệm (Multi-Worker Process Pool)**: Quản lý nhiều tiến trình con đồng thời thông qua bộ nhớ chia sẻ ẩn danh (`mmap(MAP_SHARED | MAP_ANONYMOUS)`), cho phép thêm/bớt worker động.
2. **Điều khiển nhóm tiến trình (Process Group - PGID)**: Gom các worker con vào chung một Process Group để hỗ trợ phát tín hiệu diện rộng (Broadcast) `kill(-pgid, SIGSTOP)` và `kill(-pgid, SIGCONT)`.
3. **Bộ lập lịch người dùng Preemptive Round-Robin (User-Space Scheduler)**: Sử dụng đồng hồ định thời `setitimer(ITIMER_REAL)` kết hợp ngắt tín hiệu `SIGALRM` và hai tín hiệu `SIGSTOP`/`SIGCONT` để mô phỏng hoàn hảo cơ chế phân chia thời gian CPU (Time-Sharing) của hệ điều hành.
4. **Khắc phục triệt để Text Interleaving**: Chuyển hướng nhật ký của từng worker vào các file riêng biệt (`logs/worker_<id>.log`), giữ cho menu tương tác trên Terminal của cha luôn sạch sẽ và mượt mà.
5. **Nhật ký kiểm toán an toàn (Audit Trail)**: Mọi thao tác gửi/nhận tín hiệu và xác thực PID đều được ghi lại với timestamp chi tiết tại `logs/audit.log`.
6. **Phòng chống Zombie & Orphan triệt để**: Sử dụng `waitpid(-1, &status, WNOHANG)` không khóa (non-blocking) kết hợp với đánh thức trước khi terminate (`SIGCONT` sau `SIGTERM`) để thu hồi 100% tài nguyên kernel.

---

## 2. Phân công nhiệm vụ nhóm 4 thành viên (Team Assignments)

| Thành viên | Vai trò | Phụ trách kỹ thuật & Module | Git Branch phụ trách |
| :--- | :--- | :--- | :--- |
| **Leader (Nguyễn Hoài Vy)**<br>24IT318 | Lead Architect & Scheduler Engine | Điều phối kiến trúc tổng thể, `src/main.c`, module lập lịch luân phiên `src/scheduler.c/.h`, xử lý ngắt `SIGALRM`, Review và Merge PRs. | `main` |
| **Thành viên 2 (Trần Văn Vũ)**<br>24IT315 | Process Pool Engineer | Vòng đời bể tiến trình: `mmap()` chia sẻ, `fork()`, worker loop, phân nhóm `setpgid()`, thu hồi zombie trong `src/process_manager.c/.h`. | `feature/process-management` |
| **Thành viên 3 (Đỗ Hoàng Vũ)**<br>24IT316 | Signal Control & Security | Gửi và xác thực tín hiệu: `kill()`, `SIGSTOP`, `SIGCONT`, `SIGTERM`, phát tín hiệu nhóm `kill(-pgid)`, ghi audit log trong `src/signal_handler.c/.h`. | `feature/signal-control` |
| **Thành viên 4 (Trần Trung Việt)**<br>24IT306 | Monitor & QA Automation | Phân tích `/proc/<PID>/status`, thiết kế bảng Dashboard đa tiến trình trong `src/monitor.c/.h`, xây dựng 5 bộ test tự động trong `tests/`. | `feature/terminal-ui` |

---

## 3. Kiến trúc hệ thống nâng cấp

```text
                      +------------------------------------------+
                      |                 main.c                   |  (Leader)
                      |  (CLI Coordinator & Interactive Menu)    |
                      +--------------------+---------------------+
                                           |
         +---------------------------------+---------------------------------+
         |                                 |                                 |
         v                                 v                                 v
+------------------------+      +------------------------+      +------------------------+
|  process_manager.c/.h  |      |      scheduler.c/.h    |      |      monitor.c/.h      |
| (TV2 - Process Pool)   |      | (Leader - Round-Robin) |      | (TV4 - /proc Telemetry)|
| - mmap() Shared Pool   |      | - setitimer() / SIGALRM|      | - Parse /proc/PID/stat |
| - Dynamic fork() Spawns|      | - Quantum Context-Sw   |      | - Multi-Worker Dashbrd |
| - setpgid() Groups     |      | - Preempt via SIGSTOP  |      | - State Interpretations|
| - Non-blocking Reaping |      | - Dispatch via SIGCONT |      +------------------------+
+-----------+------------+      +-----------+------------+
            |                               |
            +---------------+---------------+
                            |
                            v
              +----------------------------+
              |    signal_handler.c/.h     |
              | (TV3 - Advanced Controller)|
              | - PID Boundary Security    |
              | - kill(-pgid, SIGSTOP/CONT)|
              | - logs/audit.log Trail     |
              +-------------+--------------+
                            |
                            v
             +------------------------------+
             |      Linux Kernel Space      |
             |   (Process Table & /proc FS) |
             +------------------------------+
```

---

## 4. Hướng dẫn Biên dịch, Chạy và Kiểm thử

### 4.1. Lệnh Make chuẩn
```bash
# 1. Biên dịch toàn bộ dự án (-Wall -Wextra -Wpedantic, 0 warning)
make

# 2. Khởi chạy giao diện điều khiển tương tác
make run

# 3. Chạy toàn bộ 5 bộ kiểm thử tự động (100% PASS)
make test

# 4. Dọn dẹp sạch sẽ build artifacts và binaries
make clean
```

---

## 5. Hướng dẫn sử dụng Menu điều khiển 2 Chế độ

Khi khởi động bằng `make run`, hệ thống tự động sinh 3 Worker con và hiển thị Menu 2 chế độ:

```text
=================================================================
   LINUX PROCESS MANAGEMENT SYSTEM (C11 / POSIX.1-2008)          
   Subject: System Programming - HCMUT / VKU Linux Dev Team      
   Team Leader: tit-coder | Repo: linux-process-control         
   Architecture: Process Pool & Preemptive Round-Robin Scheduler 
=================================================================

[Initiating] Initializing Multi-Worker Process Pool via fork()...
[Process Manager] Initializing Process Pool (Target: 3 workers)...
[Process Manager] Spawned worker process PID: 7001 (Worker 1, PGID: 7001)
[Process Manager] Spawned worker process PID: 7002 (Worker 2, PGID: 7001)
[Process Manager] Spawned worker process PID: 7003 (Worker 3, PGID: 7001)

----------------------- [ CONTROL MENU ] ------------------------
 Workers: 3 active | PGID: 7001 | Scheduler: [OFF]
-----------------------------------------------------------------
  -- MANUAL PROCESS CONTROL --
  1. Suspend a specific Worker (Send SIGSTOP)
  2. Resume a specific Worker  (Send SIGCONT)
  3. Suspend ALL Workers       (Broadcast SIGSTOP via PGID)
  4. Resume ALL Workers        (Broadcast SIGCONT via PGID)
  5. Show Pool Status Dashboard(Real-time /proc telemetry)
  6. Manage Worker Pool        (Spawn new / Terminate worker)
  -- AUTO-SCHEDULER (ROUND-ROBIN) --
  7. Start Auto-Scheduler      (Round-Robin 2s Quantum via SIGALRM)
  8. Stop Auto-Scheduler       (Return to manual control)
  -- SYSTEM --
  9. Exit                      (Gracefully terminate all & quit)
-----------------------------------------------------------------
Enter your choice [1-9]: 
```

### Chi tiết các chức năng:
- **Phím `1` & `2`**: Tạm dừng hoặc Tiếp tục đích danh một Worker theo PID hoặc Worker ID (1-10).
- **Phím `3` & `4`**: Phát tín hiệu **Suspend All** (`kill(-pgid, SIGSTOP)`) và **Resume All** (`kill(-pgid, SIGCONT)`) tới toàn bộ nhóm.
- **Phím `5`**: Xuất bảng Dashboard thống kê realtime toàn bộ các tiến trình trong pool:
  ```text
  ========================================================================================
                         PROCESS POOL & SYSTEM MONITOR DASHBOARD                          
  ========================================================================================
    ID   PID      Process Name     Kernel State   Interpreted State    Counter  VmRSS     
  ----------------------------------------------------------------------------------------
    1    7001     worker-1         S (sleeping)   SLEEPING (Ready)     42       1152 kB   
    2    7002     worker-2         T (stopped)    STOPPED (SIGSTOP)    23       1148 kB   
    3    7003     worker-3         T (stopped)    STOPPED (SIGSTOP)    19       1148 kB   
  ----------------------------------------------------------------------------------------
    Pool: 3 active workers | Process Group (PGID): 7001 | Scheduler: ACTIVE (2s)
  ========================================================================================
  ```
- **Phím `6`**: Thêm mới worker vào pool hoặc kết thúc một worker theo PID.
- **Phím `7` (TÍNH NĂNG ĐINH)**: Bật **Auto-Scheduler (Round-Robin)**. Cứ mỗi 2 giây, ngắt `SIGALRM` kích hoạt context-switch: đóng băng worker hiện tại (`SIGSTOP`) và đánh thức worker tiếp theo (`SIGCONT`).
- **Phím `8`**: Dừng bộ lập lịch tự động, hồi sinh toàn bộ worker trở về trạng thái điều khiển thủ công.
- **Phím `9`**: Dọn dẹp sạch toàn bộ worker trong bể và thoát chương trình an toàn.

---

## 6. Kết quả Kiểm thử Tự động (Automated Test Suites)

Hệ thống tích hợp 5 kịch bản kiểm thử độc lập bao phủ 14 test cases:
1. `tests/test_process.sh`: Kiểm tra tạo worker qua `fork()` và thu hồi sạch sẽ.
2. `tests/test_signal.sh`: Kiểm tra phát tín hiệu `SIGSTOP`, `SIGCONT`, `SIGTERM` và đối chiếu `/proc`.
3. `tests/test_pool.sh`: Kiểm tra bể tiến trình, Process Group broadcast và sinh/hủy worker động.
4. `tests/test_scheduler.sh`: Kiểm tra bộ lập lịch Round-Robin và nhật ký kiểm toán.
5. `tests/test_invalid_pid.sh`: Kiểm tra từ chối PID âm, PID chữ, bảo vệ PID 1 `systemd`, input sai.

Chạy lệnh `make test`:
```text
[PASS] TEST 1: Process Creation & Cleanup
[PASS] TEST 2: Signal Control (SIGSTOP/CONT/TERM & Audit Trail)
[PASS] TEST 4: Multi-Worker Process Pool & PGID Broadcast
[PASS] TEST 5: Preemptive Round-Robin Scheduler via Signals
[PASS] TEST 3: Invalid PID & Security Boundaries
[DONE] All 5 test suites completed successfully.
```
Chi tiết từng ca kiểm thử xem tại [docs/test_cases.md](docs/test_cases.md).
