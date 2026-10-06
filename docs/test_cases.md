# Báo Cáo Test Cases – Process Management System

Hệ thống quản lý tiến trình bằng tín hiệu Linux Signals (`SIGSTOP`, `SIGCONT`, `SIGTERM`).

---

## Danh Sách Test Cases

| ID | Tên Test Case | Mục Tiêu | Các Bước Thực Hiện | Kết Quả Mong Đợi | Trạng Thái |
|:---|:---|:---|:---|:---|:---:|
| **TC-01** | **Start Process** | Kiểm tra tạo Worker Process bằng `fork()`. | 1. Khởi động chương trình `./process_manager`.<br>2. Quan sát log khởi tạo. | - Worker PID được cấp phát và hiển thị.<br>- Worker in vòng lặp `Running... 1`, `Running... 2`.<br>- Menu hiển thị PID và trạng thái `RUNNING`. | **PASS** |
| **TC-02** | **Suspend Process** | Tạm dừng tiến trình bằng `SIGSTOP`. | 1. Chọn menu `1` (Suspend Process).<br>2. Quan sát đầu ra của Worker. | - Tín hiệu `SIGSTOP` được gửi thành công.<br>- Worker ngừng in log tăng biến đếm `Running...`.<br>- Thông báo `[SUCCESS] Process <PID> suspended`. | **PASS** |
| **TC-03** | **Resume Process** | Tiếp tục chạy tiến trình bằng `SIGCONT`. | 1. Sau khi Suspend, chọn menu `2` (Resume Process).<br>2. Quan sát đầu ra của Worker. | - Tín hiệu `SIGCONT` được gửi thành công.<br>- Worker tiếp tục in các số đếm tiếp theo.<br>- Thông báo `[SUCCESS] Process <PID> resumed`. | **PASS** |
| **TC-04** | **Show Status** | Kiểm tra hiển thị trạng thái chính xác từ `/proc/<PID>/status`. | 1. Chọn menu `3` (Show Process Status).<br>2. Kiểm tra khi Worker đang chạy, bị dừng, hoặc đã kết thúc. | - Hiển thị đúng trạng thái thực tế: `RUNNING`, `STOPPED`, hoặc `TERMINATED`. | **PASS** |
| **TC-05** | **Terminate Process** | Kết thúc tiến trình an toàn bằng `SIGTERM`. | 1. Chọn menu `4` (Terminate Process).<br>2. Quan sát worker và parent thu hồi tài nguyên (`waitpid`). | - Tín hiệu `SIGTERM` được gửi.<br>- Worker tiến trình kết thúc sạch sẽ, không để lại zombie.<br>- Trạng thái chuyển thành `TERMINATED`. | **PASS** |
| **TC-06** | **Invalid PID** | Xử lý lỗi khi thao tác với PID không hợp lệ hoặc không tồn tại. | 1. Gọi các hàm API với PID `<= 0` (0, -1, -999).<br>2. Gọi API với PID không tồn tại (e.g. 9999999). | - Trả về mã lỗi `-1`.<br>- Thông báo lỗi rõ ràng, không làm crash chương trình. | **PASS** |
| **TC-07** | **Invalid Menu Input** | Kiểm tra chương trình không bị crash khi nhập input sai. | 1. Nhập ký tự chữ: `abc`, `xyz`.<br>2. Nhập số ngoài dải menu: `99`, `-5`.<br>3. Nhập chuỗi rỗng hoặc bấm Enter liên tục. | - Menu bắt lỗi an toàn: `"Invalid choice. Please enter a number between 1 and 5."`<br>- Chương trình tiếp tục chạy ổn định. | **PASS** |
| **TC-08** | **Suspend Nhiều Lần** | Kiểm tra tính bất biến (idempotent) khi gửi `SIGSTOP` liên tiếp. | 1. Chọn menu `1` (Suspend).<br>2. Tiếp tục chọn menu `1` thêm lần nữa. | - Hệ thống xử lý an toàn, process vẫn ở trạng thái `STOPPED`, không phát sinh lỗi ngoại lệ. | **PASS** |
| **TC-09** | **Resume Nhiều Lần** | Kiểm tra tính bất biến khi gửi `SIGCONT` liên tiếp. | 1. Chọn menu `2` (Resume).<br>2. Tiếp tục chọn menu `2` thêm lần nữa. | - Hệ thống xử lý an toàn, process tiếp tục chạy bình thường, không phát sinh lỗi ngoại lệ. | **PASS** |
| **TC-10** | **Terminate Nhiều Lần** | Kiểm tra xử lý khi tiến trình đã kết thúc mà tiếp tục gọi Terminate. | 1. Chọn menu `4` (Terminate).<br>2. Tiếp tục chọn menu `4` lần 2, lần 3. | - Thông báo rõ: `Process <PID> is already terminated.`<br>- Không cố gửi tín hiệu đến tiến trình đã chết. | **PASS** |

---

## Hướng Dẫn Chạy Tự Động Toàn Bộ Test Suite

Chạy các kịch bản kiểm thử tự động trong thư mục `tests/`:

```bash
# 1. Kiểm thử vòng đời tiến trình
bash tests/test_process.sh

# 2. Kiểm thử tín hiệu và tính bất biến
bash tests/test_signal.sh

# 3. Kiểm thử lỗi PID và input không hợp lệ
bash tests/test_invalid_pid.sh
```
