# Test Cases Documentation - Linux Process Control

Dự án: **Process Management System – Suspend / Resume Process using Signal**  
Môn học: **Lập trình hệ thống (System Programming)**  
Nhóm: **Nhóm 4 sinh viên (Leader: tit-coder)**  
Hệ điều hành kiểm thử: **Ubuntu 24.04.4 LTS (Linux kernel 6.17 x86_64)**  
Bộ biên dịch: **GCC 13.3.0 (C11, -std=c11 -D_POSIX_C_SOURCE=200809L)**  
Kiến trúc nâng cấp: **Multi-Worker Process Pool & User-Space Preemptive Round-Robin Scheduler**  

---

## 1. Danh sách tổng quan các kịch bản kiểm thử (14 Test Cases)

| Test ID | Module | Mục tiêu kiểm thử | Script tự động | Kết quả thực tế |
| :--- | :--- | :--- | :--- | :--- |
| **TC-01** | Process Manager | Tạo tiến trình con (Worker) bằng `fork()`, cấp phát và hiển thị PID | `tests/test_process.sh` | **PASS** |
| **TC-02** | Process Manager | Dọn dẹp Worker khi cha thoát, kiểm tra phòng chống Zombie / Orphan | `tests/test_process.sh` | **PASS** |
| **TC-03** | Signal Handler | Gửi `SIGSTOP` tạm dừng Worker đích danh, kiểm chứng trạng thái `T` qua `/proc` | `tests/test_signal.sh` | **PASS** |
| **TC-04** | Signal Handler | Gửi `SIGCONT` khôi phục Worker đích danh, kiểm chứng trạng thái `S`/`R` qua `/proc` | `tests/test_signal.sh` | **PASS** |
| **TC-05** | Monitor | Đọc và bóc tách thông tin từ `/proc/<PID>/status` | `tests/test_signal.sh` | **PASS** |
| **TC-06** | Process Manager | Gửi `SIGTERM` và thu hồi Worker bằng `waitpid()` không để lại Zombie | `tests/test_signal.sh` | **PASS** |
| **TC-07** | Input & Safety | Từ chối PID âm (`-999`), bảo vệ an toàn hệ điều hành | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-08** | Input & Safety | Từ chối PID không phải số (`abc_invalid`) | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-09** | Input & Safety | Từ chối gửi tín hiệu tới PID không do chương trình quản lý (PID 1 `init`/`systemd`) | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-10** | Monitor | Xử lý an toàn khi tra cứu PID không tồn tại (`9999999`) | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-11** | Terminal UI | Xử lý menu resilient khi nhập chuỗi ký tự hoặc số ngoài dải `[1-9]` | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-12** | Process Pool | Khởi tạo bể đa tiến trình (3 Worker), cấp phát bộ nhớ chia sẻ `mmap()` | `tests/test_pool.sh` | **PASS** |
| **TC-13** | Process Group | Phát tín hiệu nhóm `Suspend All` (`SIGSTOP`) và `Resume All` (`SIGCONT`) qua PGID | `tests/test_pool.sh` | **PASS** |
| **TC-14** | Auto-Scheduler | Bộ lập lịch Round-Robin tự động chuyển ngữ cảnh bằng `SIGALRM` + `SIGSTOP`/`SIGCONT` | `tests/test_scheduler.sh` | **PASS** |

---

## 2. Chi tiết từng ca kiểm thử

### TC-01: Tạo tiến trình Worker bằng `fork()`
- **Tiền điều kiện:** Chương trình được biên dịch thành công qua `make`.
- **Các bước thực hiện:**
  1. Khởi chạy `./bin/process_control`.
  2. Quan sát dòng thông báo khởi tạo và in PID Worker.
- **Kết quả mong đợi:**
  - `fork()` thành công, PID trả về > 0.
  - Tiến trình con in dòng thông báo bắt đầu và ghi heartbeat vào log riêng.
- **Kết quả thực tế:** **PASS**. Worker được spawn với PID hợp lệ, không gây text interleaving trên menu.

---

### TC-02: Thu hồi tiến trình con, chống Zombie và Orphan
- **Tiền điều kiện:** Worker đang hoạt động bình thường.
- **Các bước thực hiện:**
  1. Gửi lệnh thoát (Menu `9. Exit`).
  2. Kiểm tra trạng thái `/proc/<Worker_PID>/status` và bảng tiến trình hệ thống qua `ps`.
- **Kết quả mong đợi:**
  - Tiến trình cha gửi `SIGTERM` và gọi `waitpid()` thu hồi exit status của Worker.
  - Worker kết thúc hoàn toàn; không xuất hiện tiến trình ở trạng thái `Z` (Zombie) hoặc bị bỏ rơi (Orphan).
- **Kết quả thực tế:** **PASS**. Tiến trình con được thu hồi sạch sẽ.

---

### TC-03: Tạm dừng tiến trình bằng `SIGSTOP` (Suspend)
- **Tiền điều kiện:** Worker đang hoạt động ở trạng thái `S` (Sleeping) hoặc `R` (Running).
- **Các bước thực hiện:**
  1. Chọn Menu `1. Suspend a specific Worker` hoặc chạy `--test-signal SIGSTOP <PID>`.
  2. Kiểm tra trường `State:` trong `/proc/<Worker_PID>/status`.
- **Kết quả mong đợi:**
  - System call `kill(worker_pid, SIGSTOP)` trả về 0.
  - Trạng thái trong `/proc/<Worker_PID>/status` là `T (stopped)`.
- **Kết quả thực tế:** **PASS**. File status ghi nhận `State: T (stopped)`.

---

### TC-04: Tiếp tục tiến trình bằng `SIGCONT` (Resume)
- **Tiền điều kiện:** Worker đang bị tạm dừng (State `T`).
- **Các bước thực hiện:**
  1. Chọn Menu `2. Resume a specific Worker` hoặc chạy `--test-signal SIGCONT <PID>`.
  2. Kiểm tra lại `/proc/<Worker_PID>/status`.
- **Kết quả mong đợi:**
  - System call `kill(worker_pid, SIGCONT)` trả về 0.
  - Trạng thái tiến trình chuyển sang `S (sleeping)` hoặc `R (running)`.
  - Bộ đếm counter tiếp tục tăng từ giá trị trước khi tạm dừng.
- **Kết quả thực tế:** **PASS**. Trạng thái chuyển sang `S/R`, nhịp tim tiếp tục đếm chính xác.

---

### TC-05: Đọc và phân tích thông tin `/proc/<PID>/status`
- **Tiền điều kiện:** Worker đang tồn tại trong hệ thống.
- **Các bước thực hiện:**
  1. Chạy `--test-status <PID>` hoặc chọn Menu `5. Show Pool Dashboard`.
- **Kết quả mong đợi:**
  - Đọc thành công `/proc/<PID>/status`.
  - Hiển thị bảng định dạng chuẩn gồm: `PID`, `Process Name`, `Parent PID (PPid)`, `Raw State`, `Interpreted State`, `Virtual Memory`, `Resident Set (RSS)`, `Thread Count`.
- **Kết quả thực tế:** **PASS**. Bảng trạng thái hiển thị đầy đủ, chính xác.

---

### TC-06: Kết thúc tiến trình bằng `SIGTERM` (Terminate)
- **Tiền điều kiện:** Worker đang chạy hoặc đang suspend.
- **Các bước thực hiện:**
  1. Chọn Terminate Worker hoặc chạy `--test-signal SIGTERM <PID>`.
- **Kết quả mong đợi:**
  - Gửi `SIGTERM`, Worker nhận signal và kết thúc nhẹ nhàng với exit status 0.
  - Tiến trình cha gọi `waitpid()` thu hồi ngay lập tức.
- **Kết quả thực tế:** **PASS**. Worker terminated gracefully và được thu hồi sạch sẽ.

---

### TC-07 đến TC-11: An toàn & Xác thực đầu vào (Safety & Validation)
- **TC-07 (PID âm):** Nhập `-999` -> Hệ thống từ chối ngay, ghi audit log `REJECTED_INVALID_PID`. Kết quả: **PASS**.
- **TC-08 (PID chữ):** Nhập `abc_invalid` -> Hệ thống phát hiện lỗi định dạng, từ chối an toàn. Kết quả: **PASS**.
- **TC-09 (PID 1 - init):** Thử can thiệp vào PID 1 -> Bị chặn với cảnh báo `Security Violation: Target PID 1 (system init/systemd) is protected!`. Kết quả: **PASS**.
- **TC-10 (PID không tồn tại):** Tra cứu `/proc/9999999/status` -> Báo lỗi `Process does not exist` an toàn, không crash. Kết quả: **PASS**.
- **TC-11 (Menu invalid):** Nhập chuỗi ký tự hoặc số ngoài `[1-9]` -> Báo lỗi nhập sai, không lặp vô tận. Kết quả: **PASS**.

---

### TC-12: Quản lý Bể đa tiến trình (Process Pool via `mmap`)
- **Tiền điều kiện:** Khởi động hệ thống hoặc chạy `tests/test_pool.sh`.
- **Các bước thực hiện:**
  1. Khởi tạo pool với 3 worker đồng thời.
  2. Bổ sung động Worker 4.
  3. Thu hồi Worker 4.
- **Kết quả mong đợi:**
  - Tất cả các worker được lưu trữ trong bộ nhớ chia sẻ `mmap(MAP_SHARED | MAP_ANONYMOUS)`.
  - Bộ đếm của từng worker được cập nhật độc lập mà không bị xung đột bộ nhớ.
- **Kết quả thực tế:** **PASS**. Bể tiến trình hoạt động ổn định, thêm/xóa worker trơn tru.

---

### TC-13: Điều khiển nhóm bằng Process Group (PGID Broadcast)
- **Tiền điều kiện:** 3 Worker đang chạy trong cùng một Process Group (PGID).
- **Các bước thực hiện:**
  1. Chọn `3. Suspend ALL Workers` (`kill(-pgid, SIGSTOP)`).
  2. Kiểm tra `ps`: toàn bộ các worker đều ở trạng thái `T`.
  3. Chọn `4. Resume ALL Workers` (`kill(-pgid, SIGCONT)`).
  4. Kiểm tra `ps`: toàn bộ các worker đều chuyển lại trạng thái `S/R`.
- **Kết quả mong đợi:**
  - Một lệnh duy nhất điều khiển đồng bộ toàn bộ tiến trình trong Process Group.
- **Kết quả thực tế:** **PASS**. Tất cả worker đồng loạt dừng và đồng loạt tiếp tục.

---

### TC-14: Bộ lập lịch người dùng Preemptive Round-Robin via `SIGALRM`
- **Tiền điều kiện:** 3 Worker đang ở trong Pool.
- **Các bước thực hiện:**
  1. Kích hoạt Auto-Scheduler (Menu 7 hoặc `tests/test_scheduler.sh`).
  2. Quan sát chuyển ngữ cảnh mỗi Time Quantum (2 giây) bằng tín hiệu:
     - Worker hiện tại bị gửi `SIGSTOP` $\rightarrow$ State `T`.
     - Worker kế tiếp nhận `SIGCONT` $\rightarrow$ State `S/R`.
- **Kết quả mong đợi:**
  - Bộ định thời `setitimer(ITIMER_REAL)` phát `SIGALRM` đều đặn.
  - Các worker luân phiên nhau độc quyền thực thi CPU theo đúng chu kỳ 1 $\rightarrow$ 2 $\rightarrow$ 3 $\rightarrow$ 1.
  - Toàn bộ hành vi được ghi nhận trong `logs/audit.log` với các thẻ `SCHED_PREEMPT` và `SCHED_DISPATCH`.
- **Kết quả thực tế:** **PASS**. Luân chuyển ngữ cảnh chính xác 100%, không bị lệch pha hay rò rỉ tiến trình.
