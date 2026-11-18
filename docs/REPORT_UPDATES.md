# REPORT UPDATE PACKAGE (ENGLISH – READY TO COPY INTO WORD)
**Project:** Process Management – Suspending and Resuming Processes Using Signals in C  
**Institution:** Vietnam–Korea University of Information and Communication Technology (VKU) – Faculty of Computer Science  
**Lecturer:** PhD. Hoang Huu Duc  
**Team:** Nguyen Hoai Vy (24IT318), Tran Van Vu (24IT315), Do Hoang Vu (24IT316), Tran Trung Viet (24IT306)  
**Codebase Version:** Upgraded Architecture (Multi-Worker Process Pool, Process Group PGID, User-Space Preemptive Round-Robin Scheduler via `SIGALRM`)

---

## PART A: UPDATED TASK ASSIGNMENT TABLE (Replace old table on page ii)

| No. | Full Name | Technical Role | Files & Branch | Core Responsibilities | Result |
| :---: | :--- | :--- | :--- | :--- | :---: |
| 1 | **Nguyen Hoai Vy**<br>(Team Leader)<br>24IT318 | Lead Architect & Scheduler Engine Engineer | • `src/main.c`<br>• `src/scheduler.c`<br>• `src/scheduler.h`<br>• `Makefile`<br>*(Branch: `main`)* | • Architected five-module C11/POSIX system and 9-option dual-mode CLI.<br>• Engineered User-Space Preemptive Round-Robin Scheduler using `setitimer(ITIMER_REAL)` and async-signal-safe `SIGALRM` context switching.<br>• Merged feature branches, finalized build automation and dual-terminal demo. | 100% |
| 2 | **Tran Van Vu**<br>24IT315 | Process Pool & Lifecycle Engineer | • `src/process_manager.c`<br>• `src/process_manager.h`<br>*(Branch: `feature/process-management`)* | • Designed `worker_pool_t` in anonymous shared memory via `mmap(MAP_SHARED \| MAP_ANONYMOUS)`.<br>• Implemented dynamic `fork()` pool, `setpgid()` grouping, and private per-worker logging (`logs/worker_<id>.log`) eliminating text interleaving.<br>• Engineered non-blocking `waitpid(WNOHANG)` zombie reaping and `SIGTERM`+`SIGCONT` wake-up teardown. | 100% |
| 3 | **Do Hoang Vu**<br>24IT316 | Signal Control & Security Engineer | • `src/signal_handler.c`<br>• `src/signal_handler.h`<br>*(Branch: `feature/signal-control`)* | • Implemented targeted `SIGSTOP`/`SIGCONT`/`SIGTERM` and group broadcast (`Suspend All`/`Resume All`).<br>• Enforced strict PID boundary validation guarding PID 1 (`systemd`) and unmanaged PIDs.<br>• Built persistent timestamped audit logging in `logs/audit.log`. | 100% |
| 4 | **Tran Trung Viet**<br>24IT306 | Monitor Dashboard & QA Engineer | • `src/monitor.c`<br>• `src/monitor.h`<br>• `tests/*.sh`<br>*(Branch: `feature/terminal-ui`)* | • Built real-time Multi-Process Pool Dashboard parsing `/proc/<PID>/status` telemetry (`State`, `VmSize`, `VmRSS`, `Threads`) alongside shared counters.<br>• Authored 5 automated Bash integration test suites covering 14 test cases (100% PASS). | 100% |

---

## PART B: UPDATES FOR CHAPTER 1 (Keep existing text, update 1.3 & 1.5)

### Update 1.3. Scope of the Project
- **Functional Scope:** The system manages an elastic **Multi-Worker Process Pool** (initializing 3 concurrent background Worker processes by default, scalable up to `MAX_WORKERS = 10`) sharing a unified Process Group (`PGID`) and an anonymous shared-memory state structure (`mmap`). Workers increment per-second execution counters logged privately to `logs/worker_<id>.log`. The system operates in two modes: **Manual Control Mode** (individual or group-wide Suspend/Resume/Terminate) and **Auto-Scheduler Mode** (preemptive Round-Robin scheduling using `SIGALRM` with a 2-second time quantum).
- **User Interface Scope:** User interaction is facilitated through a clean, 9-option dual-mode Command-Line Interface (CLI) with an integrated real-time Pool Status Dashboard, completely free of console text interleaving.
- **Signal Scope:** Standard POSIX reliable signals (`SIGSTOP`, `SIGCONT`, `SIGTERM`, `SIGKILL`, `SIGINT`, and `SIGALRM` for interval timer interrupts).
- **Platform Scope:** Ubuntu Linux 24.04 LTS (x86_64) with Linux Kernel 6.17.0-generic.

### Update 1.5. Research and Implementation Methodology (bullets 2 & 4)
- **Modular System Decomposition:** Decomposing the system into **five** decoupled modules: CLI Coordination (`main.c`), Process Pool Lifecycle (`process_manager.c/.h`), Signal Control & Audit (`signal_handler.c/.h`), Preemptive Round-Robin Scheduler (`scheduler.c/.h`), and Procfs Multi-Process Telemetry (`monitor.c/.h`).
- **Automated Test-Driven Verification:** Authoring **five** automated Bash test scripts (`test_process.sh`, `test_signal.sh`, `test_pool.sh`, `test_scheduler.sh`, `test_invalid_pid.sh`) covering **14 comprehensive acceptance test cases**.

---

## PART C: MISSING THEORETICAL ADDITIONS FOR CHAPTER 2

### Add to Section 2.3: Process Groups (PGID) and Group Signaling
In POSIX systems, related processes can be aggregated into a **Process Group**, identified by a unique **Process Group Identifier (`PGID`)**. The process whose PID equals the PGID is designated the *Process Group Leader*. A process assigns itself or its child to a group via the `setpgid(pid_t pid, pid_t pgid)` system call. To prevent race conditions between parent and child scheduling after `fork()`, both parent and child invoke `setpgid()`. Process groups enable atomic broadcast signaling: passing a negative group ID (`-pgid`) to `kill(-pgid, sig)` instructs the Linux kernel to deliver `sig` simultaneously to every member of that group—forming the foundation of Unix job control and our system's `Suspend ALL` / `Resume ALL` operations.

### Add to Section 2.4: Inter-Process Shared Memory via `mmap(MAP_SHARED | MAP_ANONYMOUS)`
While `fork()` isolates memory modifications between parent and child via Copy-On-Write (COW), real-time process pool monitoring requires the parent coordinator to inspect live execution counters updated by background workers without polluting standard output. POSIX provides anonymous shared memory mapping via `mmap()`:
```c
worker_pool_t *g_pool = mmap(NULL, sizeof(worker_pool_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
```
Because the mapping is established prior to `fork()` with `MAP_SHARED`, the kernel maps the same physical memory pages into the address space of both the parent and every spawned child. Counter increments (`g_pool->workers[slot].counter++`) performed by workers become instantaneously visible to the parent's Monitor Dashboard with zero serialization overhead.

### Update Table 1 (Section 2.7): Add `SIGALRM` Row
| Signal Name | Numeric ID (x86_64) | Default Action | Catchable / Maskable? | Role in Project Architecture |
| :--- | :---: | :--- | :--- | :--- |
| **SIGSTOP** | 19 | Stop Process | NO (Uncatchable, Unignorable) | Forcibly suspends target Worker(s); kernel sets state to `'T'`. |
| **SIGCONT** | 18 | Continue Execution | YES | Awakens suspended Worker(s); kernel transitions state to `'S'` or `'R'`. |
| **SIGTERM** | 15 | Terminate | YES | Requests graceful Worker termination, allowing log flush and exit 0. |
| **SIGKILL** | 9 | Terminate | NO (Uncatchable, Unignorable) | Escalation fallback if a worker fails to respond to `SIGTERM`. |
| **SIGINT** | 2 | Terminate | YES | Intercepted by parent on `Ctrl+C` to clean up all pool workers safely. |
| **SIGALRM** | **14** | **Terminate** | **YES** | **Delivered periodically by `setitimer(ITIMER_REAL)` to trigger Round-Robin context switches.** |

### Add New Section 2.13: POSIX Interval Timers and User-Space Preemptive Round-Robin Scheduling
Modern multitasking operating systems achieve concurrency on limited CPU cores through **Preemptive Time-Sharing Scheduling**, most famously **Round-Robin (RR)** scheduling. Each runnable process is allocated a fixed **Time Quantum** (time slice). When the quantum expires, a hardware timer interrupt preempts the running task, saves its execution context, and dispatches the next process in the ready queue.

In user space, this kernel mechanism can be faithfully simulated by combining POSIX interval timers with job-control signals:
- **`setitimer(ITIMER_REAL, &itv, NULL)`**: Arms a real-time kernel timer that decrements in wall-clock time and automatically reloads at every interval (`it_interval`), raising `SIGALRM` upon expiration without the single-shot limitation of `alarm()`.
- **Signal-Driven Context Switch**: Inside the `SIGALRM` handler (restricted strictly to async-signal-safe system calls such as `kill()` and `write()`), the scheduler preempts the active worker via `kill(old_pid, SIGSTOP)` (freezing its context in State `'T'`) and dispatches the next active worker in circular queue order via `kill(new_pid, SIGCONT)` (transitioning it to State `'S'`/`'R'`).

---

## PART D: UPDATED CHAPTER 3 (SYSTEM ARCHITECTURE AND IMPLEMENTATION)

### 3.1.1. Updated Functional Requirements (Add FR-09, FR-10, FR-11 & update FR-01..FR-08)
- **FR-01 (Multi-Worker Pool Initialization):** On startup, the system initializes an anonymous shared-memory `worker_pool_t` via `mmap()` and spawns 3 concurrent child Worker processes via `fork()`, grouped under a shared `PGID` via `setpgid()`. Workers log ticks privately to `logs/worker_<id>.log` to eliminate terminal text interleaving.
- **FR-02 (Targeted & Group Suspension):** Users may suspend an individual Worker by PID/ID (Menu 1) or broadcast `SIGSTOP` to all active Workers in the pool (Menu 3).
- **FR-03 (Targeted & Group Resumption):** Users may resume an individual Worker (Menu 2) or broadcast `SIGCONT` to all active Workers (Menu 4).
- **FR-04 (Multi-Process Procfs Dashboard):** Menu 5 queries `/proc/<PID>/status` for every active Worker in the pool, rendering a consolidated real-time telemetry table (`ID`, `PID`, `Name`, `Kernel State`, `Interpreted State`, `Shared Counter`, `VmRSS`) plus PGID and Scheduler status.
- **FR-05 (Dynamic Pool Scaling & Graceful Termination):** Menu 6 enables runtime spawning of additional Workers (up to `MAX_WORKERS = 10`) and graceful termination of specific Workers via `SIGTERM` + `SIGCONT` wake-up and non-blocking `waitpid()`.
- **FR-06 (Safe Exit Teardown):** Selecting Exit (Menu 9) or issuing `Ctrl+C` (`SIGINT`) automatically disarms the scheduler, sends `SIGTERM` followed by `SIGCONT` to the process group, and reaps all children to prevent Zombies and Orphans.
- **FR-07 (Security Boundary Enforcement & Audit Logging):** Strict PID validation blocks negative PIDs, non-numeric inputs, unmanaged PIDs, and system-critical PID 1 (`systemd`). Every signal action and rejection is persisted with timestamps in `logs/audit.log`.
- **FR-08 (Automated CLI Test Dispatcher):** Supports headless flags (`--test-signal`, `--test-status`, `--test-invalid-pid`, `--test-pool`, `--test-scheduler`) and non-TTY detection (`isatty()`) for automated integration testing.
- **FR-09 (User-Space Preemptive Round-Robin Scheduler):** Menu 7 activates an autonomous scheduler using `setitimer(ITIMER_REAL)` (default 2-second quantum) and `SIGALRM` to preemptively alternate active workers via `SIGSTOP`/`SIGCONT`. Menu 8 stops the scheduler and restores all workers to manual control.

### 3.2. High-Level Five-Module Modular Architecture
Replace the old 4-layer description with the upgraded **5-module architecture**:
```text
                      +------------------------------------------+
                      |                 main.c                   |  (Leader)
                      |  (CLI Coordinator & 9-Option Dual Menu)  |
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
              | - Targeted & PGID Broadcast|
              | - logs/audit.log Trail     |
              +-------------+--------------+
                            |
                            v
             +------------------------------+
             |      Linux Kernel Space      |
             |   (Process Table & /proc FS) |
             +------------------------------+
```

### 3.3.2. Updated Table 3: Repository Source Files and Modular Responsibilities (Exact Line Counts)

| No. | Filename | Directory | Lines | Modular Engineering Responsibility |
| :---: | :--- | :--- | :---: | :--- |
| 1 | `main.c` | `src/` | 356 | Coordinator module: VKU banner, 9-option dual-mode menu, CLI test dispatcher. |
| 2 | `process_manager.c` | `src/` | 409 | Process Pool engine: `mmap()` shared pool, `fork()`, `setpgid()`, private worker logs, `waitpid()` reaper. |
| 3 | `process_manager.h` | `src/` | 127 | `worker_node_t`, `worker_pool_t` definitions and pool lifecycle APIs. |
| 4 | `scheduler.c` | `src/` | 244 | User-space Preemptive Round-Robin engine: `setitimer()`, async-signal-safe `SIGALRM` switcher. |
| 5 | `scheduler.h` | `src/` | 77 | Public interface declarations for scheduler control and time quantum configuration. |
| 6 | `signal_handler.c` | `src/` | 188 | Signal controller: `validate_target_pid()`, targeted & PGID broadcast signals, `logs/audit.log`. |
| 7 | `signal_handler.h` | `src/` | 64 | Public interface declarations for signal dispatching and audit logging. |
| 8 | `monitor.c` | `src/` | 181 | Telemetry reader: `/proc/<PID>/status` parser and Multi-Process Pool Dashboard renderer. |
| 9 | `monitor.h` | `src/` | 62 | `process_info_t` structure and monitoring dashboard APIs. |
| 10 | `Makefile` | Root | 62 | Automated GNU Make build system (`-Wall -Wextra -Wpedantic`, 5 test suites). |
| 11 | `test_process.sh` | `tests/` | 86 | Integration test verifying `fork()`, PID tracking, and zero zombie/orphan leak. |
| 12 | `test_signal.sh` | `tests/` | 123 | Integration test validating `SIGSTOP` (`T`), `SIGCONT` (`S`), `SIGTERM`, and `audit.log`. |
| 13 | `test_pool.sh` | `tests/` | 56 | Integration test validating 3-worker pool, Suspend/Resume All, dynamic spawn/terminate. |
| 14 | `test_scheduler.sh` | `tests/` | 55 | Integration test validating Round-Robin context switching and `SCHED_PREEMPT`/`SCHED_DISPATCH` logs. |
| 15 | `test_invalid_pid.sh` | `tests/` | 104 | Security test verifying negative PID, string PID, PID 1 protection, and menu resilience. |
| | **Total** | | **2,194** | **Complete modular C11/POSIX codebase & automated verification suite.** |

### 3.4. New Algorithm Subsections to Add/Update in Chapter 3

#### 3.4.1. Shared-Memory Process Pool & PGID Grouping (`pm_pool_init`, `pm_pool_spawn_worker`)
1. Allocate `worker_pool_t` via `mmap(NULL, sizeof(worker_pool_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)`.
2. Before `fork()`, call `fflush(stdout)` and `fflush(stderr)` to prevent stdio buffer duplication across children.
3. In both parent and child contexts, invoke `setpgid()` to bind all spawned workers to a single Process Group (`g_pool->pgid`), preventing race conditions.
4. Workers write per-second heartbeats exclusively to `logs/worker_<id>.log` while atomically incrementing `g_pool->workers[slot].counter` in shared memory.

#### 3.4.5. Wake-Up Graceful Termination Protocol (`SIGTERM` + `SIGCONT`)
A critical Linux kernel subtlety addressed in `pm_pool_terminate_worker()` and `pm_pool_cleanup_all()` is terminating a **stopped** process (`State T`). Under POSIX semantics, a stopped process queues `SIGTERM` and cannot execute its signal handler until resumed. Therefore, immediately after dispatching `kill(pid, SIGTERM)` (or `kill(-pgid, SIGTERM)`), the manager issues `kill(pid, SIGCONT)` (or `kill(-pgid, SIGCONT)`). This wakes suspended workers instantaneously so they run their `SIGTERM` handler, flush log files, and exit cleanly with status `0` without requiring forcible `SIGKILL` escalation.

#### 3.4.8. User-Space Preemptive Round-Robin Scheduler Algorithm (`src/scheduler.c`)
1. **Initialization (`scheduler_start`)**: Broadcast `SIGSTOP` to suspend all active workers, trigger an initial tick to dispatch Worker 1 via `SIGCONT`, register `scheduler_alrm_handler` via `sigaction(SIGALRM, &sa, NULL)` with `SA_RESTART`, and arm a 2-second periodic timer via `setitimer(ITIMER_REAL, &itv, NULL)`.
2. **Async-Signal-Safe Context Switch (`scheduler_step_tick`)**: On each `SIGALRM` interrupt:
   - Preempt the currently running worker via `kill(old_pid, SIGSTOP)` (State `'T'`).
   - Advance circularly (`(start + i) % MAX_WORKERS`) to locate the next active worker.
   - Dispatch CPU time slice via `kill(new_pid, SIGCONT)` (State `'S'`/`'R'`).
   - Emit an async-signal-safe status notification directly to `STDOUT_FILENO` using custom `safe_itoa()` and `write()` (avoiding non-reentrant `printf()` inside signal context).
3. **Teardown (`scheduler_stop`)**: Zero out `struct itimerval` to disarm `ITIMER_REAL`, restore `SIG_DFL` on `SIGALRM`, and broadcast `SIGCONT` to return all workers to manual control.

### 3.5. Additions to Table 4 (Core System Calls and POSIX APIs)
Add these 4 rows to your existing Table 4:

| Function / API | Header File | Classification | Engineering Purpose in Codebase |
| :--- | :--- | :--- | :--- |
| `mmap()` | `<sys/mman.h>` | System Call | Allocates anonymous shared memory (`MAP_SHARED \| MAP_ANONYMOUS`) for `worker_pool_t`. |
| `setpgid()` | `<unistd.h>` | System Call | Assigns all spawned workers into a unified Process Group (`PGID`) for broadcast signaling. |
| `setitimer()` | `<sys/time.h>` | System Call | Arms periodic real-time interval timer (`ITIMER_REAL`) raising `SIGALRM` every 2 seconds. |
| `isatty()` | `<unistd.h>` | POSIX API | Detects interactive terminal vs. piped test streams to prevent input blocking in CI tests. |

---

## PART E: UPDATED CHAPTER 4 (TESTING, EXPERIMENTAL RESULTS, AND EVALUATION)

### Fix Numbering in Chapter 4
Ensure your Table of Contents and headings follow strict sequential numbering:
- `4.3. Process Pool Creation and Shared-Memory Verification`
- `4.4. Targeted & Group Suspension Verification (SIGSTOP & PGID Broadcast)`
- `4.5. Process Resumption Verification (SIGCONT & State S/R)` *(Fix missing 4.5 in old TOC!)*
- `4.6. Real-Time Multi-Process Pool Dashboard via /proc`
- `4.7. User-Space Preemptive Round-Robin Scheduler Verification (SIGALRM)` *(NEW!)*
- `4.8. Audit Trail Verification (logs/audit.log)` *(NEW!)*
- `4.9. Graceful Termination (SIGTERM+SIGCONT) and Zero-Zombie Verification`
- `4.10. Automated Integration Test Suites Execution (make test – 5 Suites, 14 Test Cases)`
- `4.11. Technical Evaluation and Goal Achievement Matrix`
- `4.12. Current System Limitations and Future Enhancements`

### Updated Table 7: System Acceptance Test Matrix (TC-01 through TC-14)

| Test ID | Target Module | Test Objective | Input / Action | Expected Result | Actual Result | Status |
| :---: | :--- | :--- | :--- | :--- | :--- | :---: |
| **TC-01** | Process Manager | Spawn Worker via `fork()` | Launch application | Child spawned with valid PID > 0; logs heartbeats | Spawned with valid PID; private log active | **PASS** |
| **TC-02** | Process Manager | Prevent Zombies on Exit | Select Menu 9 (Exit) | Parent reaps all children via `waitpid()`; no zombies | All children reaped; `/proc/<PID>` removed | **PASS** |
| **TC-03** | Signal Handler | Suspend via `SIGSTOP` | `--test-signal SIGSTOP` / Menu 1 | Dispatches `SIGSTOP`; `/proc` reports State `T` | Kernel `/proc/<PID>/status` reports `State: T` | **PASS** |
| **TC-04** | Signal Handler | Resume via `SIGCONT` | `--test-signal SIGCONT` / Menu 2 | Dispatches `SIGCONT`; State becomes `S`/`R` | State returns to `S`; counter continues | **PASS** |
| **TC-05** | Monitor | Parse `/proc/<PID>/status` | `--test-status` / Menu 5 | Formatted Dashboard: PID, State, Counter, VmRSS | Telemetry table displayed accurately | **PASS** |
| **TC-06** | Process Manager | Terminate via `SIGTERM` | `--test-signal SIGTERM` / Menu 6 | Child wakes (`SIGCONT`), handles `SIGTERM`, exits 0 | Worker terminated gracefully (exit code 0) | **PASS** |
| **TC-07** | Input Validation | Reject Negative PIDs | `--test-invalid-pid -999` | Rejects negative value; logs rejection | Reports invalid PID; aborts cleanly | **PASS** |
| **TC-08** | Input Validation | Reject Non-Numeric PIDs | `--test-invalid-pid abc_invalid` | Rejects malformed string input | Reports malformed PID; rejects safely | **PASS** |
| **TC-09** | Security | Protect PID 1 (`systemd`) | `--test-invalid-pid 1` | Detects protected PID 1; flags Security Violation | Reports Security Violation; blocked | **PASS** |
| **TC-10** | Monitor | Query Non-Existent PID | `--test-status 9999999` | Reports process does not exist without crashing | Safely reports process not found | **PASS** |
| **TC-11** | Terminal UI | Handle Invalid Menu Input | Inputs: `'invalid'`, `'99'`, `'-5'` | Emits `[Invalid Input]` warning; no infinite loop | Warns user; menu re-renders cleanly | **PASS** |
| **TC-12** | Process Pool | Multi-Worker Pool & `mmap` | `tests/test_pool.sh` | Spawns 3 workers in shared pool; dynamic add/remove | 3 initial + 4th dynamic worker verified | **PASS** |
| **TC-13** | Process Group | Broadcast via PGID | Menu 3 (`Suspend ALL`) & Menu 4 | All pool workers toggle `T` and `S` simultaneously | `ps` confirms all 3 workers `T` -> `S` | **PASS** |
| **TC-14** | Auto-Scheduler | Round-Robin via `SIGALRM` | Menu 7 & `tests/test_scheduler.sh` | Workers alternate `SIGSTOP`/`SIGCONT` every 2s | Audit log & `watch ps` confirm 2s rotation | **PASS** |

### Updated Section 4.12 (Replaces old 4.11 which listed ALREADY-SOLVED limitations!)
#### 4.12.1. Identified Limitations (Updated)
- **Fixed Pool Capacity:** The shared-memory pool uses a compile-time cap (`MAX_WORKERS = 10`) rather than dynamically resizable shared segments (`mremap()`).
- **Uniform Synthetic Workload:** Current workers execute uniform 1-second timer workloads rather than heterogeneous CPU-bound (e.g., prime factorization) vs. I/O-bound workloads.
- **Line-Oriented Terminal UI:** Although text interleaving has been eliminated via per-worker log files, the interface remains a standard scrolling CLI rather than a full-screen interactive split-pane TUI.

#### 4.12.2. Roadmap for Future Development
- **Dynamic CPU Throttling (`cpulimit`-style Duty Cycling):** Combining `/proc/<PID>/stat` CPU utilization measurements with sub-second `SIGSTOP`/`SIGCONT` duty cycles to cap CPU-bound workers at target percentages (e.g., 30% CPU).
- **Ncurses Split-Screen TUI Dashboard:** Integrating `libncurses` to display live worker tables, real-time CPU/RAM sparklines, and single-keystroke shortcuts simultaneously.
- **Linux `cgroups v2` Integration:** Pairing POSIX signal orchestration with kernel Control Groups v2 (`cpu.max`, `memory.max`) for hard container-grade resource isolation.

### Additional References to Append to REFERENCES
- `[13]` Linux Kernel Organization, *Linux Programmer's Manual: mmap(2) — Map or Unmap Files or Devices into Memory*. Ubuntu Manpages, 2024. [Online]. Available: https://man7.org/linux/man-pages/man2/mmap.2.html
- `[14]` Linux Kernel Organization, *Linux Programmer's Manual: setitimer(2) — Get or Set Value of an Interval Timer*. Ubuntu Manpages, 2024. [Online]. Available: https://man7.org/linux/man-pages/man2/setitimer.2.html
- `[15]` Linux Kernel Organization, *Linux Programmer's Manual: setpgid(2) — Set/Get Process Group*. Ubuntu Manpages, 2024. [Online]. Available: https://man7.org/linux/man-pages/man2/setpgid.2.html

---

## PART F: EXACT SCREENSHOT CHECKLIST (COMMANDS TO CAPTURE FRESH FIGURES)

> **IMPORTANT:** Before taking screenshots, run `pkill -9 -f process_control 2>/dev/null || true` so Terminal 2 shows **zero** old leftover processes, and note that `src/main.c` now displays **`VKU Faculty of Computer Science`** (fixing the old `HCMUT` typo in your previous screenshots!).

### Group 1: Code & Architecture Screenshots (Chapter 3)
1. **Figure 1 (Updated Directory Tree):**
   ```bash
   cd ~/linux-process-control && tree -L 3 -I 'build|bin'
   ```
2. **Figure 2 (Shared Memory Pool & `fork()` + `setpgid()` in `src/process_manager.c`):**
   ```bash
   cat -n src/process_manager.c | sed -n '103,190p'
   ```
3. **Figure 3 (Decoupled Worker Heartbeat Loop in `src/process_manager.c`):**
   ```bash
   cat -n src/process_manager.c | sed -n '39,101p'
   ```
4. **Figure 4 (Signal Validation, PGID Broadcast & Audit Log in `src/signal_handler.c`):**
   ```bash
   cat -n src/signal_handler.c | sed -n '46,110p'
   ```
5. **Figure 5 (NEW – Preemptive Round-Robin Scheduler in `src/scheduler.c`):**
   ```bash
   cat -n src/scheduler.c | sed -n '57,136p'
   cat -n src/scheduler.c | sed -n '145,193p'
   ```
6. **Figure 6 (Multi-Process Dashboard in `src/monitor.c`):**
   ```bash
   cat -n src/monitor.c | sed -n '120,181p'
   ```
7. **Figure 7 (Updated `Makefile` with 5 Test Suites):**
   ```bash
   cat -n Makefile
   ```

### Group 2: Live Dual-Terminal Demo Screenshots (Chapter 4)
1. **Clean Build (`0 warnings`):**
   ```bash
   cd ~/linux-process-control && make clean && make
   ```
2. **Startup Banner (VKU) + 3-Worker Pool Initialization + 9-Option Menu (Terminal 1):**
   ```bash
   make run
   ```
3. **Process Tree & Unified PGID Proof (Terminal 2):**
   ```bash
   ps -o pid,ppid,pgid,stat,cmd -C process_control
   ```
4. **Multi-Process Pool Dashboard (Terminal 1 – press `5`):** Capture the ASCII dashboard table showing `worker-1`, `worker-2`, `worker-3`, `State: S`, `Counter`, `VmRSS`.
5. **Suspend ALL via PGID (Terminal 1 – press `3` + Terminal 2 `ps -o pid,stat,cmd -C process_control`):** Capture all 3 workers displaying `STAT: T`.
6. **Resume ALL via PGID (Terminal 1 – press `4`):** Capture confirmation + Terminal 2 showing `STAT: S`.
7. **Auto-Scheduler Round-Robin Live Rotation (Terminal 1 – press `7` + Terminal 2 `watch -n 1 "ps -o pid,stat,cmd -C process_control"`):**
   - Capture Terminal 1 printing `[Scheduler SIGALRM] Time Quantum expired! Switched to Worker 1 -> 2 -> 3`.
   - Capture Terminal 2 showing **1 worker in `S` and 2 workers in `T`**!
8. **Audit Trail Proof (Terminal 2):**
   ```bash
   cat logs/audit.log | tail -n 12
   ```
9. **Graceful Exit (`9`) & Zero-Zombie Proof (Terminal 1 + Terminal 2 `ps -ef | grep process_control`):** All 3 workers exit with status `0`.
10. **5 Automated Integration Test Suites (Terminal 1):**
    ```bash
    make test
    ```
11. **Updated Git Status & History:**
    ```bash
    git status && git log -n 4 --oneline
    ```
