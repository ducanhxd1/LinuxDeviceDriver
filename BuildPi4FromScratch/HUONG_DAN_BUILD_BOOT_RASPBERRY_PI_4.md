# Hướng dẫn build và boot Linux 64-bit cho Raspberry Pi 4 từ đầu

> Mục tiêu của tài liệu: giúp bạn tự tạo một thẻ microSD có thể boot Raspberry Pi 4, hiểu từng tầng của boot flow, biết cách thu log và báo cáo kết quả khi mentor giao task setup/bring-up board.
>
> Quy trình mặc định dùng **Raspberry Pi firmware boot thẳng Linux kernel**. Đây là đường ngắn nhất để có lần boot đầu tiên thành công. Phần **U-Boot** được làm sau, như một bài nâng cao có kiểm soát.

---

## 1. Kết quả cuối cùng cần đạt

Sau khi hoàn thành, bạn phải có:

- Kernel Linux ARM64 tự cross-compile cho BCM2711.
- Device Tree và overlays đi cùng kernel.
- Debian 13 `trixie` ARM64 root filesystem tối giản.
- Thẻ microSD gồm một phân vùng FAT32 để boot và một phân vùng ext4 làm rootfs.
- Serial console 115200 baud để quan sát log từ sớm.
- Ethernet DHCP và SSH để tiếp tục phát triển.
- Một bộ thông tin có thể gửi mentor: commit source, kernel config, sơ đồ phân vùng, log boot và checklist test.

Đây là hệ thống phục vụ học tập, bring-up và phát triển driver. Nó chưa mặc nhiên là một image production: chưa có reproducible package snapshot, secure boot, read-only rootfs, rollback hay OTA.

---

## 2. Đọc tài liệu gốc thế nào cho đúng

Tài liệu `Build_OS_RaspberryPi4.md` mô tả đúng các khối lớn: toolchain, firmware/U-Boot, kernel, rootfs và SD card. Tuy nhiên, khi thực hành lần đầu nên điều chỉnh như sau:

1. Boot trực tiếp bằng firmware trước, chỉ thêm U-Boot sau khi đường cơ bản đã hoạt động.
2. Dùng `PARTUUID` cho tham số `root=` thay vì giả định root luôn là `/dev/mmcblk0p2`.
3. Pi 4 dùng bootloader trong SPI EEPROM; không cần `bootcode.bin` cho boot bình thường.
4. MBR là lựa chọn tương thích và đơn giản cho bài này, không nên diễn đạt là định dạng duy nhất Pi 4 có thể dùng.
5. Kernel, DTB và modules phải đến từ cùng một lần build. Không lấy DTB ngẫu nhiên từ firmware để ghép với kernel khác nếu đang debug lỗi.
6. Driver thẻ SD và ext4 phải được build-in (`=y`) nếu không dùng initramfs.

---

## 3. Kiến thức nền: Pi 4 boot như thế nào?

Đường boot mặc định trong tài liệu này:

```text
BCM2711 Boot ROM
  -> SPI EEPROM bootloader
  -> tìm phân vùng boot FAT
  -> start4.elf + fixup4.dat
  -> đọc config.txt và cmdline.txt
  -> nạp bcm2711-rpi-4-b.dtb + overlays
  -> nạp kernel8.img
  -> Linux mount rootfs theo PARTUUID
  -> chạy /sbin/init (systemd)
  -> serial login / Ethernet / SSH
```

Ba khái niệm rất hay bị trộn lẫn:

| Thành phần | Chạy ở đâu | Nhiệm vụ |
|---|---|---|
| EEPROM bootloader | SPI EEPROM trên Pi 4 | Chọn SD/USB/network theo `BOOT_ORDER`, tìm firmware |
| VideoCore firmware | File trên FAT: `start4.elf`, `fixup4.dat` | Đọc cấu hình, chuẩn bị DT, nạp Arm kernel |
| Linux kernel | `kernel8.img` | Khởi tạo CPU/driver, mount rootfs, chạy init |

U-Boot là bootloader tầng hai tùy chọn, nằm giữa firmware và Linux.

---

## 4. Phần cứng và phần mềm cần chuẩn bị

### 4.1 Phần cứng

- Raspberry Pi 4 Model B.
- Nguồn USB-C 5.1 V / 3 A tốt.
- microSD 8 GB tối thiểu; nên dùng 16 GB trở lên.
- Đầu đọc microSD cho máy host.
- Cáp Ethernet và mạng có DHCP.
- USB-to-TTL **3.3 V** dùng chip CP2102/FT232/CH340; tuyệt đối không đưa mức 5 V vào GPIO Pi.
- Tùy chọn: màn hình micro-HDMI và bàn phím USB.

### 4.2 Máy host

Giả định máy build là Ubuntu/Debian x86_64, có Internet, còn ít nhất 25 GB và có quyền `sudo`.

Kiểm tra:

```bash
uname -m
df -h .
```

`uname -m` nên trả về `x86_64`. Nếu host đã là ARM64, không cần QEMU cho chroot và có thể native-build, nhưng tài liệu này vẫn tập trung vào cross-build từ x86_64.

### 4.3 Cài công cụ

```bash
sudo apt update
sudo apt install -y \
  bc binfmt-support bison build-essential ca-certificates cpio \
  crossbuild-essential-arm64 debootstrap device-tree-compiler \
  dosfstools e2fsprogs flex git kmod libncurses-dev libssl-dev \
  parted pkg-config qemu-user-static rsync screen u-boot-tools \
  wget xz-utils
```

Kiểm tra toolchain và QEMU:

```bash
aarch64-linux-gnu-gcc --version
qemu-aarch64-static --version
```

Không dùng prefix `arm-linux-gnueabihf-`: đó là ARM 32-bit, trong khi bài này là AArch64.

---

## 5. Tạo workspace và ghi lại phiên bản

Không build lẫn lộn trong nhiều thư mục. Dùng một workspace rõ ràng:

```bash
export PI4_WORK="$HOME/pi4-lab"
export PI4_SRC="$PI4_WORK/src"
export PI4_OUT="$PI4_WORK/out"
export PI4_KBUILD="$PI4_OUT/kernel-build"
export PI4_BOOT="$PI4_OUT/boot"
export PI4_ROOTFS="$PI4_OUT/rootfs"
export PI4_JOBS="$(nproc)"

mkdir -p "$PI4_SRC" "$PI4_KBUILD" "$PI4_BOOT" "$PI4_ROOTFS"
```

Trong một shell mới, các biến trên không còn tồn tại. Hãy khai báo lại hoặc lưu chúng vào một file script riêng. Trước mỗi bước nguy hiểm, kiểm tra:

```bash
printf '%s\n' "$PI4_WORK" "$PI4_SRC" "$PI4_OUT" "$PI4_ROOTFS"
```

Tài liệu dùng:

```bash
export PI4_KERNEL_BRANCH="rpi-6.12.y"
export PI4_DEBIAN_SUITE="trixie"
export PI4_DEBIAN_MIRROR="https://deb.debian.org/debian"
```

Nhánh có thể di chuyển theo thời gian. Với task nghiêm túc, mentor cần cung cấp tag/commit; nếu không, bạn phải ghi lại commit thực tế sau khi clone.

---

## 6. Lấy Raspberry Pi firmware

```bash
cd "$PI4_SRC"
git clone --depth=1 https://github.com/raspberrypi/firmware.git
cd firmware
git rev-parse HEAD | tee "$PI4_OUT/firmware.commit"
```

Stage toàn bộ nội dung boot trước để tránh thiếu file firmware phụ:

```bash
rsync -a boot/ "$PI4_BOOT/"
```

Sau này ta sẽ ghi đè kernel, DTB và overlays bằng kết quả kernel tự build.

Pi 4 không cần `bootcode.bin` trong luồng boot bình thường. Nếu Pi hoàn toàn không đọc thẻ dù thẻ chuẩn, hãy dùng Raspberry Pi Imager tạo thẻ EEPROM recovery/SD boot; không tự sửa EEPROM ngay trong bước build OS.

---

## 7. Build Linux kernel cho Raspberry Pi 4

### 7.1 Clone source

```bash
cd "$PI4_SRC"
git clone --depth=1 --branch "$PI4_KERNEL_BRANCH" \
  https://github.com/raspberrypi/linux.git
cd linux
git rev-parse HEAD | tee "$PI4_OUT/kernel.commit"
```

### 7.2 Tạo cấu hình

Raspberry Pi duy trì `bcm2711_defconfig` cho Pi 4:

```bash
cd "$PI4_SRC/linux"
make O="$PI4_KBUILD" \
  ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  bcm2711_defconfig
```

Tùy chọn mở giao diện cấu hình:

```bash
make O="$PI4_KBUILD" ARCH=arm64 menuconfig
```

Với lần boot không có initramfs, phải kiểm tra các khả năng thiết yếu được build-in. Lệnh sau chỉ kiểm tra, không tự sửa:

```bash
grep -E '^(CONFIG_(DEVTMPFS|DEVTMPFS_MOUNT|MMC|MMC_BLOCK|EXT4_FS|SERIAL_AMBA_PL011_CONSOLE))=' \
  "$PI4_KBUILD/.config"
```

Các mục lưu trữ/rootfs thiết yếu phải là `y`, không phải `m`. Nếu một symbol tên khác ở kernel mới, dùng tìm kiếm `/` trong `menuconfig`, đọc `Help`, sau đó lưu config.

### 7.3 Compile

```bash
cd "$PI4_SRC/linux"
make O="$PI4_KBUILD" \
  ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  -j"$PI4_JOBS" Image.gz modules dtbs
```

Build có thể mất từ vài phút đến lâu hơn tùy CPU/RAM. Kiểm tra artifact:

```bash
file "$PI4_KBUILD/arch/arm64/boot/Image.gz"
ls -lh "$PI4_KBUILD/arch/arm64/boot/Image.gz"
find "$PI4_KBUILD/arch/arm64/boot/dts" \
  -name 'bcm2711-rpi-4-b.dtb' -print
```

### 7.4 Copy kernel, DTB và overlays vào staging

```bash
cp "$PI4_KBUILD/arch/arm64/boot/Image.gz" \
  "$PI4_BOOT/kernel8.img"

cp "$PI4_KBUILD/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb" \
  "$PI4_BOOT/bcm2711-rpi-4-b.dtb"

mkdir -p "$PI4_BOOT/overlays"
rsync -a --delete \
  "$PI4_KBUILD/arch/arm64/boot/dts/overlays/" \
  "$PI4_BOOT/overlays/"
```

Nếu DTB không nằm đúng path trên, dùng kết quả `find` ở bước trước. Không dùng DTB của một lần build khác.

Lưu kernel config để mentor có thể tái hiện:

```bash
cp "$PI4_KBUILD/.config" "$PI4_OUT/kernel.config"
```

---

## 8. Tạo Debian ARM64 rootfs

### 8.1 Bootstrap giai đoạn một và hai

`ROOTFS` phải trống trước khi chạy lần đầu. Không dùng lệnh xóa nếu bạn chưa xác nhận đúng biến/thư mục.

```bash
test -n "$PI4_ROOTFS" && printf 'ROOTFS=%s\n' "$PI4_ROOTFS"
sudo mkdir -p "$PI4_ROOTFS"

sudo debootstrap --arch=arm64 --foreign --variant=minbase \
  "$PI4_DEBIAN_SUITE" "$PI4_ROOTFS" "$PI4_DEBIAN_MIRROR"

sudo cp /usr/bin/qemu-aarch64-static "$PI4_ROOTFS/usr/bin/"
sudo chroot "$PI4_ROOTFS" /debootstrap/debootstrap --second-stage
```

Nếu `Exec format error`, kiểm tra `binfmt-support`, QEMU và thử:

```bash
sudo update-binfmts --enable qemu-aarch64
```

### 8.2 Chuẩn bị DNS và virtual filesystems cho chroot

```bash
sudo cp -L /etc/resolv.conf "$PI4_ROOTFS/etc/resolv.conf"
sudo mount --bind /dev "$PI4_ROOTFS/dev"
sudo mount --bind /dev/pts "$PI4_ROOTFS/dev/pts"
sudo mount -t proc proc "$PI4_ROOTFS/proc"
sudo mount -t sysfs sys "$PI4_ROOTFS/sys"
sudo mount -t tmpfs tmpfs "$PI4_ROOTFS/run"
```

### 8.3 Khai báo repository Debian

```bash
sudo tee "$PI4_ROOTFS/etc/apt/sources.list.d/debian.sources" >/dev/null <<EOF
Types: deb
URIs: $PI4_DEBIAN_MIRROR
Suites: $PI4_DEBIAN_SUITE $PI4_DEBIAN_SUITE-updates
Components: main
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg

Types: deb
URIs: https://security.debian.org/debian-security
Suites: $PI4_DEBIAN_SUITE-security
Components: main
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg
EOF
```

### 8.4 Cấu hình hệ thống cơ bản

```bash
sudo chroot "$PI4_ROOTFS" /bin/bash
```

Từ đây đến lệnh `exit`, bạn đang ở **bên trong rootfs ARM64**:

```bash
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  systemd-sysv udev sudo openssh-server \
  iproute2 iputils-ping systemd-resolved \
  ca-certificates kmod locales less vim-tiny

echo pi4-lab > /etc/hostname
cat > /etc/hosts <<'EOF'
127.0.0.1 localhost
127.0.1.1 pi4-lab
::1 localhost ip6-localhost ip6-loopback
EOF

echo 'en_US.UTF-8 UTF-8' > /etc/locale.gen
locale-gen
ln -sf /usr/share/zoneinfo/Asia/Ho_Chi_Minh /etc/localtime
echo Asia/Ho_Chi_Minh > /etc/timezone

useradd -m -s /bin/bash pi
usermod -aG sudo,dialout,video,input,plugdev pi
passwd pi

systemctl enable ssh
systemctl enable systemd-networkd
systemctl enable systemd-resolved

cat > /etc/systemd/network/20-wired.network <<'EOF'
[Match]
Name=eth*

[Network]
DHCP=yes
EOF

ln -sf /run/systemd/resolve/stub-resolv.conf /etc/resolv.conf
apt-get clean
rm -rf /var/lib/apt/lists/*
exit
```

`passwd pi` là tương tác và không hiện ký tự khi gõ. Không ghi mật khẩu rõ vào script hoặc Git. Nếu image dùng trong công việc, nên khóa password và cài SSH public key theo chính sách của dự án.

### 8.5 Cài kernel modules vào đúng rootfs

Chạy lại trên host:

```bash
sudo make -C "$PI4_SRC/linux" O="$PI4_KBUILD" \
  ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH="$PI4_ROOTFS" modules_install
```

Kiểm tra:

```bash
find "$PI4_ROOTFS/lib/modules" -mindepth 1 -maxdepth 1 -type d -print
```

### 8.6 Thoát chroot sạch sẽ

```bash
sudo umount "$PI4_ROOTFS/dev/pts"
sudo umount "$PI4_ROOTFS/dev"
sudo umount "$PI4_ROOTFS/proc"
sudo umount "$PI4_ROOTFS/sys"
sudo umount "$PI4_ROOTFS/run"
sudo rm -f "$PI4_ROOTFS/usr/bin/qemu-aarch64-static"
```

Nếu báo `target is busy`, kiểm tra bằng `findmnt -R "$PI4_ROOTFS"`; không tháo thẻ hoặc xóa rootfs khi các mount còn tồn tại.

---

## 9. Ghi hệ thống vào microSD

> **CẢNH BÁO:** phần này xóa bảng phân vùng và dữ liệu trên thiết bị đã chọn. `/dev/sdX` chỉ là placeholder. Chọn sai có thể xóa ổ hệ điều hành của máy host.

### 9.1 Xác định đúng thiết bị

Trước khi cắm thẻ:

```bash
lsblk -dpno NAME,SIZE,MODEL,TRAN
```

Cắm thẻ rồi chạy lại. Dựa vào thiết bị mới xuất hiện, model và kích thước để đặt biến, ví dụ:

```bash
export PI4_SD_DEVICE=/dev/sdX
lsblk -o NAME,SIZE,MODEL,TYPE,FSTYPE,MOUNTPOINTS "$PI4_SD_DEVICE"
```

Biến phải là **whole disk** như `/dev/sdb`, không phải `/dev/sdb1`. Dừng lại và xác nhận trực quan trước khi tiếp tục.

### 9.2 Tạo hai phân vùng

Unmount các phân vùng do desktop tự mount, sau đó tạo MBR để tương thích đơn giản:

```bash
sudo umount "${PI4_SD_DEVICE}"?* 2>/dev/null || true
sudo wipefs --all "$PI4_SD_DEVICE"
sudo parted --script "$PI4_SD_DEVICE" \
  mklabel msdos \
  mkpart primary fat32 1MiB 513MiB \
  set 1 boot on \
  mkpart primary ext4 513MiB 100%
sudo partprobe "$PI4_SD_DEVICE"
```

Tên partition khác nhau giữa USB reader và `mmcblk`/NVMe:

```bash
if [[ "$PI4_SD_DEVICE" == /dev/mmcblk* || "$PI4_SD_DEVICE" == /dev/nvme* ]]; then
  export PI4_SD_BOOT="${PI4_SD_DEVICE}p1"
  export PI4_SD_ROOT="${PI4_SD_DEVICE}p2"
else
  export PI4_SD_BOOT="${PI4_SD_DEVICE}1"
  export PI4_SD_ROOT="${PI4_SD_DEVICE}2"
fi

printf 'BOOT=%s\nROOT=%s\n' "$PI4_SD_BOOT" "$PI4_SD_ROOT"
lsblk "$PI4_SD_DEVICE"
```

Format:

```bash
sudo mkfs.vfat -F 32 -n PI_BOOT "$PI4_SD_BOOT"
sudo mkfs.ext4 -L PI_ROOT "$PI4_SD_ROOT"
```

### 9.3 Lấy ID ổn định

```bash
export PI4_ROOT_PARTUUID="$(sudo blkid -s PARTUUID -o value "$PI4_SD_ROOT")"
export PI4_BOOT_UUID="$(sudo blkid -s UUID -o value "$PI4_SD_BOOT")"
export PI4_ROOT_UUID="$(sudo blkid -s UUID -o value "$PI4_SD_ROOT")"

printf 'ROOT_PARTUUID=%s\nBOOT_UUID=%s\nROOT_UUID=%s\n' \
  "$PI4_ROOT_PARTUUID" "$PI4_BOOT_UUID" "$PI4_ROOT_UUID"
```

Không để bất kỳ giá trị nào rỗng.

### 9.4 Mount và copy

```bash
export PI4_MNT_BOOT="$PI4_WORK/mnt-boot"
export PI4_MNT_ROOT="$PI4_WORK/mnt-root"
sudo mkdir -p "$PI4_MNT_BOOT" "$PI4_MNT_ROOT"
sudo mount "$PI4_SD_BOOT" "$PI4_MNT_BOOT"
sudo mount "$PI4_SD_ROOT" "$PI4_MNT_ROOT"

sudo rsync -aHAX --numeric-ids "$PI4_ROOTFS/" "$PI4_MNT_ROOT/"
sudo rsync -a "$PI4_BOOT/" "$PI4_MNT_BOOT/"
```

### 9.5 Tạo cấu hình direct boot

`config.txt`:

```bash
sudo tee "$PI4_MNT_BOOT/config.txt" >/dev/null <<'EOF'
[all]
arm_64bit=1
kernel=kernel8.img
enable_uart=1
EOF
```

`cmdline.txt` phải chỉ có **một dòng vật lý**, không có comment hoặc xuống dòng giữa các tham số:

```bash
sudo tee "$PI4_MNT_BOOT/cmdline.txt" >/dev/null <<EOF
console=serial0,115200 console=tty1 root=PARTUUID=$PI4_ROOT_PARTUUID rootfstype=ext4 rootwait rw fsck.repair=yes
EOF
```

Tạo mount point và `fstab`:

```bash
sudo mkdir -p "$PI4_MNT_ROOT/boot/firmware"
sudo tee "$PI4_MNT_ROOT/etc/fstab" >/dev/null <<EOF
UUID=$PI4_ROOT_UUID / ext4 defaults,noatime 0 1
UUID=$PI4_BOOT_UUID /boot/firmware vfat defaults 0 2
EOF
```

### 9.6 Kiểm tra trước khi tháo thẻ

```bash
ls -lh \
  "$PI4_MNT_BOOT/start4.elf" \
  "$PI4_MNT_BOOT/fixup4.dat" \
  "$PI4_MNT_BOOT/kernel8.img" \
  "$PI4_MNT_BOOT/bcm2711-rpi-4-b.dtb"

wc -l "$PI4_MNT_BOOT/cmdline.txt"
cat "$PI4_MNT_BOOT/cmdline.txt"
cat "$PI4_MNT_ROOT/etc/fstab"
test -x "$PI4_MNT_ROOT/sbin/init"
find "$PI4_MNT_ROOT/lib/modules" -mindepth 1 -maxdepth 1 -type d -print
```

`wc -l` phải cho kết quả `1`. Cuối cùng:

```bash
sync
sudo umount "$PI4_MNT_ROOT"
sudo umount "$PI4_MNT_BOOT"
sudo sync
```

Chỉ tháo thẻ sau khi `umount` thành công.

---

## 10. Nối UART và boot lần đầu

### 10.1 Đấu dây

Tắt nguồn Pi trước khi nối:

| Header Pi | Tín hiệu Pi | Nối với USB-TTL |
|---:|---|---|
| Pin 6 | GND | GND |
| Pin 8 | GPIO14 / TXD | RXD |
| Pin 10 | GPIO15 / RXD | TXD |

Không nối chân 5 V của adapter vào Pi. Logic phải là 3.3 V.

Tìm serial port trên host:

```bash
dmesg --follow
```

Thường là `/dev/ttyUSB0` hoặc `/dev/ttyACM0`. Mở terminal:

```bash
sudo screen /dev/ttyUSB0 115200
```

Thiết lập là 115200, 8 data bits, no parity, 1 stop bit, no flow control. Thoát `screen`: `Ctrl-A`, rồi `K`, xác nhận `y`.

### 10.2 Trình tự boot mong đợi

1. Cắm SD.
2. Cắm Ethernet.
3. Mở serial terminal.
4. Cấp nguồn.
5. Quan sát kernel log.
6. Chờ prompt `pi4-lab login:` và đăng nhập user `pi`.

Các mốc quan trọng:

```text
Booting Linux on physical CPU ...
Machine model: Raspberry Pi 4 Model B ...
Kernel command line: ... root=PARTUUID=...
VFS: Mounted root (ext4 filesystem) ...
Run /sbin/init as init process
systemd[1]: ...
pi4-lab login:
```

### 10.3 Checklist xác nhận sau login

```bash
uname -a
uname -m
cat /proc/device-tree/model; echo
cat /proc/cmdline
findmnt /
findmnt /boot/firmware
lsblk -f
ip -brief link
ip -brief address
systemctl --failed
journalctl -b -p warning
```

Kỳ vọng:

- `uname -m` là `aarch64`.
- Model là Raspberry Pi 4.
- `/` là ext4 trên partition thứ hai.
- Ethernet có địa chỉ DHCP nếu dây và DHCP server hoạt động.
- `systemctl --failed` không có service quan trọng bị fail.

Lấy IP rồi SSH từ host:

```bash
ssh pi@<IP_CUA_PI>
```

---

## 11. Troubleshooting theo tầng boot

Nguyên tắc quan trọng: xác định **mốc cuối cùng đã chạy**, không sửa đồng thời mọi thứ.

### 11.1 Không có ACT LED hợp lý, không HDMI, không serial

Tập trung vào nguồn, thẻ, EEPROM và FAT boot:

- Thử nguồn và SD đã biết là tốt.
- Mount thẻ trên host, kiểm tra `start4.elf`, `fixup4.dat`, `config.txt`.
- Kiểm tra FAT bằng `sudo fsck.fat -vn "$PI4_SD_BOOT"` khi partition không mount.
- Khôi phục/cập nhật EEPROM bằng Raspberry Pi Imager nếu board cũ hoặc EEPROM lỗi.
- Serial Linux chưa thể hiện gì nếu firmware chưa hề nạp kernel; HDMI diagnostics có thể hữu ích hơn ở tầng này.

### 11.2 Firmware không tìm thấy kernel

- `config.txt` phải trỏ đúng `kernel=kernel8.img`.
- File phải nằm ở root của phân vùng FAT.
- `file kernel8.img` phải tương ứng kernel ARM64 nén, không phải `zImage` ARM32 hay binary x86.
- Kiểm tra tên file phân biệt chữ hoa/thường.

### 11.3 Kernel chạy nhưng panic `VFS: Unable to mount root fs`

- So sánh `root=PARTUUID=...` trong `cmdline.txt` với `blkid` thật.
- Phải có `rootwait`.
- `CONFIG_MMC`, driver MMC/SDHCI phù hợp và `CONFIG_EXT4_FS` phải là `y`.
- Chạy `sudo e2fsck -fn "$PI4_SD_ROOT"` khi partition đã unmount.
- Không trộn kernel với modules/DTB từ build khác.

### 11.4 Kernel mount root nhưng `No working init found`

Kiểm tra trên host:

```bash
ls -l "$PI4_ROOTFS/sbin/init"
file "$PI4_ROOTFS/lib/systemd/systemd"
```

Rootfs phải là ARM64 và có `systemd-sysv`. Nếu executable tồn tại nhưng không chạy, kiểm tra dynamic loader và thư viện trong rootfs.

### 11.5 Có boot nhưng không thấy serial login

- Kiểm tra TX/RX đã bắt chéo, chung GND, mức 3.3 V.
- Kiểm tra `enable_uart=1` và `console=serial0,115200`.
- Nếu thấy kernel log nhưng không có login prompt:

```bash
systemctl status serial-getty@serial0.service
sudo systemctl enable --now serial-getty@serial0.service
```

### 11.6 Ethernet không có IP

```bash
ip link
networkctl
systemctl status systemd-networkd
journalctl -b -u systemd-networkd
dmesg | grep -i -E 'bcmgenet|ethernet|firmware'
```

Kiểm tra driver BCMGENET trong kernel, Device Tree, dây mạng và DHCP server. Wi-Fi cần thêm firmware Broadcom và cấu hình riêng; hãy làm Ethernet thành công trước.

### 11.7 Boot treo hoặc reset ngẫu nhiên

- Đổi nguồn và cáp USB-C trước.
- Tháo thiết bị USB tiêu thụ dòng lớn.
- Kiểm tra cảnh báo undervoltage trong log.
- Dùng SD chất lượng tốt và chạy filesystem check.

---

## 12. Thêm U-Boot sau khi direct boot đã thành công

U-Boot hữu ích khi task yêu cầu boot menu, nhiều kernel, TFTP/PXE, recovery hoặc A/B update.

### 12.1 Build

```bash
cd "$PI4_SRC"
git clone https://source.denx.de/u-boot/u-boot.git
cd u-boot
git checkout <TAG_DUOC_MENTOR_CHOT>
git rev-parse HEAD | tee "$PI4_OUT/u-boot.commit"

make distclean
make rpi_arm64_defconfig
make -j"$PI4_JOBS" CROSS_COMPILE=aarch64-linux-gnu-
cp u-boot.bin "$PI4_BOOT/u-boot.bin"
```

Nếu project/phiên bản U-Boot chính thức yêu cầu `rpi_4_defconfig`, dùng defconfig được mentor hoặc board documentation của đúng tag chỉ định. Ghi lại lựa chọn và lý do.

### 12.2 Chuyển cấu hình trên FAT sang U-Boot

Mount partition boot, copy `u-boot.bin`, rồi đổi `config.txt`:

```ini
[all]
arm_64bit=1
kernel=u-boot.bin
enable_uart=1
```

Tạo `extlinux/extlinux.conf`:

```text
DEFAULT linux
TIMEOUT 20

LABEL linux
    LINUX /kernel8.img
    FDT /bcm2711-rpi-4-b.dtb
    APPEND console=serial0,115200 console=tty1 root=PARTUUID=<ROOT_PARTUUID_THAT> rootfstype=ext4 rootwait rw fsck.repair=yes
```

Thay placeholder bằng `PARTUUID` thật. Không để literal dấu `< >`.

Khi debug, log bây giờ chia thành ba mốc: firmware đã gọi U-Boot chưa; U-Boot đã tìm `extlinux.conf` và kernel chưa; kernel đã mount rootfs chưa.

---

## 13. Khi mentor giao task, cần hỏi và báo cáo gì?

### 13.1 Làm rõ đầu bài trước khi build

- Board chính xác: Pi 4B revision/RAM hay CM4? SD hay eMMC?
- Kiến trúc 32-bit hay 64-bit?
- Kernel tree, branch, tag hoặc commit nào?
- Boot trực tiếp, U-Boot, initramfs hay NFS root?
- Rootfs: Debian, BusyBox, Buildroot hay Yocto?
- Driver/peripheral bắt buộc: UART, I2C, SPI, CAN, Wi-Fi, camera...?
- Output mong muốn: thẻ SD, `.img`, script tái lập, patch hay tài liệu?
- Tiêu chí pass: login serial, SSH, device node, test driver hay thời gian boot?

Nếu mentor không chỉ định, ghi rõ giả định của bạn thay vì âm thầm chọn.

### 13.2 Evidence nên lưu

```bash
mkdir -p "$PI4_OUT/evidence"
cp "$PI4_OUT"/*.commit "$PI4_OUT/evidence/" 2>/dev/null || true
cp "$PI4_OUT/kernel.config" "$PI4_OUT/evidence/"
sha256sum \
  "$PI4_BOOT/kernel8.img" \
  "$PI4_BOOT/bcm2711-rpi-4-b.dtb" \
  "$PI4_BOOT/start4.elf" \
  "$PI4_BOOT/fixup4.dat" \
  > "$PI4_OUT/evidence/boot.sha256"
```

Trên target, lưu:

```bash
uname -a
cat /proc/device-tree/model; echo
cat /proc/cmdline
lsblk -f
systemctl --failed
journalctl -b
```

Dùng `script` hoặc terminal logging để lưu toàn bộ UART boot log ngay từ lần đầu.

### 13.3 Mẫu báo cáo ngắn

```text
Board: Raspberry Pi 4 Model B, <RAM/revision>
Boot medium: microSD
Boot flow: EEPROM -> RPi firmware -> Linux (direct boot)
Firmware commit: ...
Kernel branch/commit: ...
Kernel config: đính kèm
Rootfs: Debian 13 trixie arm64, debootstrap minbase
Partition: p1 FAT32 512 MiB, p2 ext4 remaining
Pass: serial login, root ext4, Ethernet DHCP, SSH
Known issue: ...
Artifacts: image/checksum/log/config/patch ...
```

---

## 14. Tạo file image để giao nộp (tùy chọn)

Sau khi thẻ đã test thành công và mọi partition đã unmount, có thể sao lưu toàn bộ thiết bị. File sẽ lớn bằng dung lượng thẻ:

```bash
sudo dd if="$PI4_SD_DEVICE" of="$PI4_OUT/pi4-working.img" \
  bs=4M status=progress conv=fsync
xz -T0 -v "$PI4_OUT/pi4-working.img"
sha256sum "$PI4_OUT/pi4-working.img.xz" \
  > "$PI4_OUT/pi4-working.img.xz.sha256"
```

`dd` với `if=` đọc từ thẻ. Đảo nhầm `if` và `of` có thể phá dữ liệu. Trong workflow chuyên nghiệp, nên tạo image từ file loopback có kích thước xác định để artifact nhỏ và tái lập được, thay vì clone một SD dung lượng lớn.

---

## 15. Các bước nâng cao sau bài đầu tiên

Thực hiện theo thứ tự tăng dần độ phức tạp:

1. Viết script build có `set -euo pipefail`, pin exact commits và tạo manifest.
2. Tạo BusyBox initramfs để tách lỗi storage/rootfs khỏi lỗi kernel.
3. Boot NFS root để phát triển driver nhanh, không phải ghi SD liên tục.
4. Dùng U-Boot TFTP để tải kernel/DTB qua mạng.
5. Chuyển sang Buildroot hoặc Yocto để quản lý rootfs và reproducibility.
6. Thêm A/B rootfs, watchdog, rollback và kiểm thử mất điện.
7. Chỉ nghiên cứu secure boot sau khi chuỗi boot thường đã ổn định và có recovery plan.

---

## 16. Tài liệu chính thức nên đọc

- Raspberry Pi boot và EEPROM: <https://www.raspberrypi.com/documentation/computers/raspberry-pi.html>
- Raspberry Pi kernel build: <https://www.raspberrypi.com/documentation/computers/linux_kernel.html>
- Raspberry Pi Linux source: <https://github.com/raspberrypi/linux>
- Raspberry Pi firmware: <https://github.com/raspberrypi/firmware>
- U-Boot Raspberry Pi board documentation: <https://docs.u-boot.org/en/latest/board/broadcom/raspberrypi.html>
- Debian 13 trixie: <https://www.debian.org/releases/trixie/>
- `debootstrap` manual: <https://manpages.debian.org/trixie/debootstrap/debootstrap.8.en.html>

---

## 17. Checklist cuối cùng

```text
[ ] Đúng Raspberry Pi 4 / BCM2711 / ARM64
[ ] Ghi lại firmware và kernel commit
[ ] Kernel build thành công: Image.gz, modules, dtbs
[ ] kernel8.img + DTB cùng một kernel build
[ ] Rootfs có /sbin/init và modules đúng version
[ ] SD_DEVICE được kiểm tra bằng lsblk trước khi ghi
[ ] FAT boot có start4.elf, fixup4.dat, config.txt, cmdline.txt
[ ] cmdline.txt đúng một dòng và PARTUUID đúng
[ ] fstab dùng UUID đúng
[ ] SD được sync và unmount sạch
[ ] UART là 3.3 V, TX/RX bắt chéo, 115200 8N1
[ ] Boot tới serial login
[ ] Root mount ext4, Ethernet DHCP và SSH hoạt động
[ ] Lưu boot log, config, commit và checksum để báo cáo mentor
```

Nếu checklist dừng ở đâu, hãy debug đúng tầng đó. Một kỹ sư bring-up tốt không chỉ làm board boot được, mà còn chỉ ra được firmware, bootloader, kernel, rootfs hay phần cứng là tầng đang lỗi và cung cấp log chứng minh.
