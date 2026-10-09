# Lời giải — Command Line

Hãy giải thích từng token trước khi chạy các lệnh mẫu.

```sh
practice_dir=$(mktemp -d)
cd "$practice_dir"
mkdir -p src include build
printf '%s\n' '/* TODO: implement */' > src/main.c
printf '%s\n' '/* TODO: add API */' > src/api.c
printf '%s\n' 'int ready = 1;' > src/state.c
rg -n 'TODO' src
```

Brace expansion `mkdir -p {src,include,build}` do shell thực hiện, không phải
tính năng của `mkdir`.

Tách stdout và stderr:

```sh
sh -c 'printf "normal\n"; printf "error\n" >&2' > build/out.log 2> build/err.log
```

Hard link và symbolic link:

```sh
printf 'payload\n' > original.txt
ln original.txt hard-link.txt
ln -s original.txt symbolic-link.txt
ls -li original.txt hard-link.txt symbolic-link.txt
```

File gốc và hard link có cùng inode. Symlink có inode riêng và chứa pathname.

Process, `/proc`, signal và thu hồi trạng thái:

```sh
sleep 300 &
sleep_pid=$!
sed -n '1,12p' "/proc/$sleep_pid/status"
kill -TERM "$sleep_pid"
wait "$sleep_pid"
printf 'status=%d\n' "$?"
```

Status thường là 143 (`128 + SIGTERM(15)`). `wait` thu hồi trạng thái process.

Năm process có RSS cao nhất:

```sh
ps -eo pid,comm,rss --sort=-rss | head -n 6
```

Dòng đầu là header. RSS có đơn vị KiB và gồm cả các shared page được tính cho
nhiều process nên không phải phép đo hoàn hảo về memory riêng.

`cd /tmp | pwd` không đổi thư mục shell hiện tại vì các thành phần pipeline chạy
trong môi trường con. Bật `set -o pipefail` để lỗi phía trái pipeline không bị
status thành công của command cuối che mất.

