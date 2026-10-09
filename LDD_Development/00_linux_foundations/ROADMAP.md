# Roadmap của chương 00

Mỗi phiên dự kiến 60–90 phút. Chỉ chuyển phiên khi tự giải thích và tái hiện
được kết quả chính.

## Phiên 1 — Shell và filesystem

- `pwd`, `ls`, `cd`, `mkdir`, `cp`, `mv`, `rm`, `find`, `rg`.
- Absolute/relative path, hidden file, inode, hard link và symbolic link.
- Redirection, pipe, exit status và quoting.
- Thực hành: `exercises/01_command_line.md`.

## Phiên 2 — Permission, process và `/proc`

- User/group, `rwx`, umask, `chmod`, `chown`.
- PID, parent/child, foreground/background, signal và exit code.
- `/proc`, `/sys` và `/dev` là giao diện quan sát kernel.

## Phiên 3 — Toolchain C

- Preprocess → compile → assemble → link.
- Header, translation unit, static/shared library và ABI cơ bản.
- `gcc`, `readelf`, `nm`, `objdump`, `ldd`.
- Build và phân tích `labs/01_system_inspector`.

## Phiên 4 — System programming

- File descriptor và `open/read/write/close`.
- Partial I/O, `errno`, EINTR, cleanup và ownership.
- Làm `labs/02_safe_file_copy`.

## Phiên 5 — Process và signal

- `fork`, `exec`, `waitpid` và async-signal safety.
- Làm `labs/03_process_signals`.

## Phiên 6 — Debug và test

- `gdb`, core dump, sanitizers, `strace`, boundary/error-path tests.
- Cố ý tạo lỗi, thu evidence, tìm root cause rồi sửa.

## Phiên 7 — Cross-compilation

- Build/host/target, target triplet, sysroot và dynamic loader.
- Làm `labs/04_cross_compile_pi4`.

