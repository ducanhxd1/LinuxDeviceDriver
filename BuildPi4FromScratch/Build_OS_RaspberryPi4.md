# Build Embedded Linux OS cho Raspberry Pi 4 (từ Scratch)

> Tài liệu này mô tả toàn bộ pipeline build 1 hệ điều hành Linux tối giản, có thể boot được trên board **Raspberry Pi 4 (BCM2711, ARM Cortex-A72, 64-bit)**, đi theo đúng sơ đồ:
>
> `[1. Toolchain] → [2. U-Boot / RPi Firmware] → [3. Linux Kernel] → [4. Rootfs (Debian/Busybox)] → [5. Chia phân vùng SD Card]`
>
> Kiến trúc mục tiêu: **aarch64 (ARM64)** — đây là kiến trúc chuẩn cho Pi 4 kể từ khi Raspberry Pi OS 64-bit trở thành bản mặc định (2022 trở về sau). Tài liệu áp dụng cho kernel nhánh `rpi-6.6.y` trở lên (nhánh đang được Raspberry Pi Ltd. duy trì tính đến 2025-2026).

---

## 0. Tổng quan cơ chế boot của Pi 4

Khác với PC thông thường, Pi 4 **không dùng BIOS/UEFI truyền thống**. Trình tự boot gồm 4 tầng:

```
[Boot ROM trong SoC BCM2711]
        │  (đọc bootloader từ SPI-EEPROM, KHÔNG còn đọc bootcode.bin trên SD như Pi3)
        ▼
[EEPROM Bootloader - rpi-eeprom]
        │  đọc config.txt, nạp GPU firmware
        ▼
[VideoCore Firmware: start4.elf + fixup4.dat]
        │  đọc config.txt/cmdline.txt, khởi tạo GPU, dựng Device Tree
        ▼
[U-Boot (u-boot.bin) hoặc trực tiếp Linux kernel8.img]
        │  U-Boot đọc extlinux.conf / boot script
        ▼
[Linux Kernel (Image) + Device Tree Blob (.dtb)]
        │  mount rootfs
        ▼
[Rootfs: init / systemd / busybox init]
```

Điểm quan trọng cần nhớ:
- Pi 4 (từ khoảng 2020 trở đi) boot **bootloader từ EEPROM** chứ không phải từ `bootcode.bin` trên thẻ SD như Pi 3. `bootcode.bin` chỉ còn cần thiết nếu bạn recovery EEPROM.
- Phân vùng boot **bắt buộc là FAT32**, nằm ở phân vùng đầu tiên của SD card — đây là giới hạn cứng của GPU firmware, không đổi qua các đời Pi.
- U-Boot là **tùy chọn**, không bắt buộc: bạn có thể để `start4.elf` load thẳng `kernel8.img`. Nhưng đưa U-Boot vào giữa cho ta: boot menu, `extlinux.conf`, mạng (tftp/pxe), khả năng chọn kernel A/B (hữu ích cho OTA sau này). Tài liệu này build đầy đủ cả U-Boot theo đúng sơ đồ yêu cầu.

---

## 1. Toolchain (Cross-Compilation Toolchain)

### 1.1 Chọn toolchain

Build trên máy host **x86_64 Linux** (Ubuntu/Debian) để tạo binary chạy trên **aarch64**. Có 2 cách:

| Cách | Ưu điểm | Nhược điểm |
|---|---|---|
| Toolchain có sẵn từ distro (`crossbuild-essential-arm64`) | Cài nhanh, đủ dùng để build kernel + U-Boot | Có thể không tối ưu hiệu năng runtime |
| Toolchain ngoài (Linaro / Bootlin / ARM GNU Toolchain) | Kiểm soát version glibc, tối ưu tốt hơn, dùng khi build rootfs từ Buildroot | Cài đặt thủ công |

### 1.2 Cài đặt (Ubuntu/Debian host)

```bash
sudo apt update
sudo apt install -y \
    git build-essential bc bison flex \
    libssl-dev libncurses5-dev \
    crossbuild-essential-arm64 \
    device-tree-compiler \
    debootstrap qemu-user-static binfmt-support \
    dosfstools e2fsprogs parted kpartx \
    python3 python3-pyelftools u-boot-tools \
    libgnutls28-dev uuid-dev
```

Gói `crossbuild-essential-arm64` cài sẵn bộ triplet **`aarch64-linux-gnu-`** (gcc, ld, objcopy...). Kiểm tra:

```bash
aarch64-linux-gnu-gcc --version
```

### 1.3 (Tùy chọn) Toolchain chuẩn hơn từ ARM GNU Toolchain

Nếu muốn kiểm soát version glibc chặt hơn (khuyến nghị khi build rootfs bằng Buildroot):

```bash
cd ~
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
tar xf arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
export PATH=$HOME/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin:$PATH
export CROSS_COMPILE=aarch64-none-linux-gnu-
```

> Cả 2 toolchain trên đều dùng chung biến `CROSS_COMPILE` khi build U-Boot và Kernel ở bước sau. Chốt 1 loại và dùng xuyên suốt để tránh lệch ABI.

---

## 2. U-Boot / RPi Firmware

Bước này build **2 thành phần độc lập** cùng nằm trên phân vùng boot: firmware GPU của Raspberry Pi (bắt buộc) và U-Boot (bootloader, tùy chọn nhưng được đưa vào theo sơ đồ).

### 2.1 Lấy RPi Firmware (GPU bootloader + closed-source blob)

Không tự build phần này (đây là firmware đóng của Broadcom/VideoCore), chỉ tải về:

```bash
git clone --depth=1 https://github.com/raspberrypi/firmware.git rpi-firmware
```

Các file cần dùng cho Pi 4 nằm trong `rpi-firmware/boot/`:
- `bootcode.bin` — chỉ dùng cho recovery EEPROM, Pi4 đời mới có thể bỏ qua nếu EEPROM đã cập nhật.
- `start4.elf`, `fixup4.dat` — GPU firmware bắt buộc cho Pi 4 (hậu tố `4` = dành riêng cho BCM2711).
- `bcm2711-rpi-4-b.dtb` — device tree gốc do firmware sinh ra (nếu không dùng DTB tự build từ kernel).

Cập nhật EEPROM bootloader (khuyến nghị, chạy trực tiếp trên Pi đang có OS, không phải bước cross-build):

```bash
sudo rpi-eeprom-update -a   # trên Pi, không phải trên máy host build
```

### 2.2 Build U-Boot từ source

```bash
git clone https://source.denx.de/u-boot/u-boot.git
cd u-boot
git checkout v2025.01   # chọn tag ổn định mới nhất tại thời điểm build

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

make rpi_arm64_defconfig     # defconfig 64-bit khuyến nghị cho Pi3/Pi4/Pi400/CM4
make -j$(nproc)
```

> Ghi chú defconfig: U-Boot cung cấp `rpi_4_defconfig` (build kèm DTB nhúng sẵn trong U-Boot) và `rpi_arm64_defconfig` (dùng DTB do firmware GPU cấp thay vì DTB nhúng). Với pipeline có kernel tự build ở bước 3 kèm DTB riêng, **nên dùng `rpi_arm64_defconfig`** để tránh xung đột device tree giữa U-Boot và kernel.

Sau khi build xong, output quan trọng nhất là `u-boot.bin` ở thư mục gốc `u-boot/`.

### 2.3 Cấu hình `config.txt` để firmware GPU nạp U-Boot thay vì nạp thẳng kernel

Trong phân vùng boot (sẽ tạo ở bước 5), file `config.txt`:

```ini
[pi4]
kernel=u-boot.bin
arm_64bit=1
enable_uart=1
```

Khi đó chuỗi boot sẽ là: `start4.elf` → `u-boot.bin` → (U-Boot đọc `extlinux.conf`) → `Image` (kernel) + `.dtb`.

### 2.4 U-Boot boot script — `extlinux.conf`

U-Boot bản mới hỗ trợ sẵn `distro_bootcmd`, tự động tìm file `/boot/extlinux/extlinux.conf` trên phân vùng boot:

```
# /boot/extlinux/extlinux.conf
menu title U-Boot Boot Menu
timeout 30

label linux
    kernel /Image
    fdt /bcm2711-rpi-4-b.dtb
    append console=serial0,115200 console=tty1 root=/dev/mmcblk0p2 rootwait rw
```

---

## 3. Linux Kernel

### 3.1 Lấy source kernel chính thức của Raspberry Pi Foundation

Fork kernel của Raspberry Pi (chứa driver/patch riêng cho BCM2711 chưa hoặc chậm upstream vào mainline):

```bash
git clone --depth=1 --branch rpi-6.6.y https://github.com/raspberrypi/linux.git
cd linux
```

> Có thể dùng kernel mainline (kernel.org) nếu chỉ cần hỗ trợ phần cứng cơ bản (CPU, RAM, MMC, Ethernet) vì phần lớn driver Pi4 đã upstream từ kernel 5.x trở đi. Nhưng để có đủ driver cho camera/VideoCore/Bluetooth/WiFi cùng patch tối ưu, nên dùng fork `raspberrypi/linux`.

### 3.2 Cấu hình kernel

```bash
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

make bcm2711_defconfig
```

`bcm2711_defconfig` là defconfig chính thức dành riêng cho SoC của Pi 4 (arch/arm64/configs/bcm2711_defconfig).

Tùy chỉnh thêm nếu cần (bật/tắt driver, chọn scheduler, PREEMPT_RT...):

```bash
make menuconfig
# hoặc nconfig / xconfig tùy giao diện thích
```

Một vài mục nên kiểm tra cho hệ thống tối giản có thể boot ổn định:
- `General setup → Initramfs/initrd support`: bật nếu dùng initramfs, tắt nếu boot thẳng vào rootfs trên SD.
- `Device Drivers → MMC/SD/SDIO card support`: bắt buộc (đọc thẻ SD).
- `File systems → The Extended 4 (ext4) filesystem`: bắt buộc nếu rootfs dùng ext4.
- `File systems → DOS/FAT/NT Filesystems → MSDOS fs / VFAT`: bắt buộc để mount phân vùng boot.
- `Device Drivers → Network device support → Ethernet/WiFi driver BCM2711`: cần nếu dùng mạng.

### 3.3 Build kernel, module, device tree

```bash
make -j$(nproc) Image modules dtbs
```

Output cần lấy:
- `arch/arm64/boot/Image` → kernel image
- `arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb` → device tree cho board Pi4-B
- `arch/arm64/boot/dts/overlays/*.dtbo` → các overlay (bật/tắt I2C, SPI, camera...)

### 3.4 Cài module vào rootfs

Bước này thực hiện **sau khi đã có rootfs** (bước 4), trỏ `INSTALL_MOD_PATH` vào thư mục rootfs đã mount:

```bash
sudo make -j$(nproc) INSTALL_MOD_PATH=/mnt/rpi-rootfs modules_install
```

---

## 4. Rootfs (Debian hoặc Busybox)

Chọn 1 trong 2 hướng tùy mục tiêu: **Debian minimal** (đầy đủ, dễ apt install thêm) hoặc **Busybox** (tối giản, dùng cho hệ thống nhúng thật sự nhỏ gọn).

### 4.A Rootfs Debian bằng `debootstrap` (khuyến nghị cho học tập / dev)

```bash
sudo mkdir -p /mnt/rpi-rootfs
sudo debootstrap --arch=arm64 --foreign bookworm /mnt/rpi-rootfs http://deb.debian.org/debian

# Do debootstrap 2 giai đoạn (host x86_64 build rootfs arm64) nên cần qemu-user-static
sudo cp /usr/bin/qemu-aarch64-static /mnt/rpi-rootfs/usr/bin/
sudo chroot /mnt/rpi-rootfs /debootstrap/debootstrap --second-stage
```

Vào chroot để cấu hình hệ thống cơ bản:

```bash
sudo chroot /mnt/rpi-rootfs /bin/bash

# --- bên trong chroot ---
echo "raspberrypi4" > /etc/hostname
passwd root                      # đặt password root
apt update
apt install -y systemd-sysv udev network-manager openssh-server sudo

# tạo user thường
adduser duc
usermod -aG sudo duc

# cấu hình fstab cho 2 phân vùng sẽ tạo ở bước 5
cat <<EOF > /etc/fstab
/dev/mmcblk0p1  /boot   vfat    defaults          0  2
/dev/mmcblk0p2  /       ext4    defaults,noatime  0  1
EOF

exit
```

### 4.B Rootfs Busybox (tối giản, phù hợp embedded thật sự)

```bash
git clone --depth=1 https://git.busybox.net/busybox
cd busybox
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

make defconfig
# Bật static build để không phụ thuộc thư viện ngoài rootfs
sed -i 's/^# CONFIG_STATIC.*/CONFIG_STATIC=y/' .config

make -j$(nproc)
make CONFIG_PREFIX=/mnt/rpi-rootfs install
```

Sau khi cài Busybox, cần tự dựng khung thư mục và `init` tối thiểu:

```bash
cd /mnt/rpi-rootfs
sudo mkdir -p dev proc sys etc/init.d root

sudo tee etc/inittab > /dev/null <<'EOF'
::sysinit:/etc/init.d/rcS
::respawn:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/swapoff -a
EOF

sudo tee etc/init.d/rcS > /dev/null <<'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev
echo "Busybox rootfs on Raspberry Pi 4 - boot OK"
EOF
sudo chmod +x etc/init.d/rcS
```

> Với Busybox, cần thêm module kernel (nếu có) vào `lib/modules/` bằng `modules_install` như bước 3.4, và không cần systemd/udev.

---

## 5. Chia phân vùng SD Card

### 5.1 Tạo bảng phân vùng

Xác định thiết bị SD (ví dụ `/dev/sdX`, **kiểm tra kỹ bằng `lsblk` trước khi thao tác** để tránh ghi nhầm ổ đĩa hệ thống):

```bash
lsblk
sudo umount /dev/sdX*   # nếu hệ thống tự mount sẵn

sudo parted /dev/sdX --script -- \
    mklabel msdos \
    mkpart primary fat32 1MiB 513MiB \
    mkpart primary ext4 513MiB 100%

sudo mkfs.vfat -F 32 -n BOOT /dev/sdX1
sudo mkfs.ext4 -L rootfs /dev/sdX2
```

Bảng phân vùng dùng **MBR (msdos)**, không phải GPT — đây vẫn là yêu cầu bắt buộc của GPU firmware Pi4 để đọc được phân vùng boot FAT32 đầu tiên.

### 5.2 Mount và copy dữ liệu vào phân vùng BOOT

```bash
sudo mkdir -p /mnt/boot /mnt/root
sudo mount /dev/sdX1 /mnt/boot
sudo mount /dev/sdX2 /mnt/root

# --- Firmware GPU (bắt buộc) ---
sudo cp rpi-firmware/boot/start4.elf /mnt/boot/
sudo cp rpi-firmware/boot/fixup4.dat /mnt/boot/
sudo cp rpi-firmware/boot/bootcode.bin /mnt/boot/    # tùy chọn, phục vụ recovery

# --- U-Boot ---
sudo cp u-boot/u-boot.bin /mnt/boot/

# --- Kernel + Device Tree ---
sudo cp linux/arch/arm64/boot/Image /mnt/boot/
sudo cp linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb /mnt/boot/
sudo mkdir -p /mnt/boot/overlays
sudo cp linux/arch/arm64/boot/dts/overlays/*.dtbo /mnt/boot/overlays/

# --- config.txt (đã soạn ở mục 2.3) ---
sudo cp config.txt /mnt/boot/

# --- extlinux.conf (đã soạn ở mục 2.4) ---
sudo mkdir -p /mnt/boot/extlinux
sudo cp extlinux.conf /mnt/boot/extlinux/
```

### 5.3 Copy rootfs

```bash
sudo rsync -aHAX /mnt/rpi-rootfs/ /mnt/root/
sync
```

### 5.4 (Nếu dùng Busybox, không dùng debootstrap) — copy module kernel

```bash
sudo make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
    INSTALL_MOD_PATH=/mnt/root modules_install
```

### 5.5 Umount an toàn

```bash
sync
sudo umount /mnt/boot
sudo umount /mnt/root
```

---

## 6. Kiểm tra boot lần đầu

1. Cắm SD card vào Pi 4, nối UART (GPIO14/15, 115200 8N1) để xem log boot sớm nhất — quan trọng vì HDMI console chỉ lên sau khi kernel init xong driver display.
2. Cấp nguồn. Trình tự log kỳ vọng: `start4.elf` in banner GPU → U-Boot in version + đếm ngược `Hit any key to stop autoboot` → U-Boot đọc `extlinux.conf` → kernel `Booting Linux on physical CPU 0x0` → mount rootfs → prompt login.
3. Lỗi thường gặp:

| Triệu chứng | Nguyên nhân phổ biến | Cách xử lý |
|---|---|---|
| Không có gì trên UART, không thấy đèn ACT nháy | `start4.elf`/`fixup4.dat` sai, hoặc thiếu `config.txt` | Kiểm tra lại tên file, đúng biến thể `4` (không phải `start.elf` của Pi cũ) |
| U-Boot lên nhưng không tìm thấy kernel | Sai đường dẫn trong `extlinux.conf`, hoặc rootfs chưa có `/boot/extlinux/` | Kiểm tra path tương đối, U-Boot tìm trong cùng phân vùng boot |
| Kernel panic `VFS: Unable to mount root fs` | Sai `root=` trong `append`, hoặc thiếu driver MMC/ext4 build vào kernel (không phải module) | Đảm bảo `CONFIG_MMC_SDHCI_*` và `CONFIG_EXT4_FS` build-in (`Y`), không để `M` nếu không có initramfs |
| Boot xong nhưng không có mạng/WiFi | Thiếu firmware blob WiFI/BT của Broadcom (không nằm trong kernel source) | Copy thêm `brcm/*.bin` từ `raspberrypi/firmware/boot/`, hoặc gói `firmware-brcm80211` nếu dùng Debian rootfs |

---

## 7. Tóm tắt cây thư mục cuối cùng trên SD Card

```
/dev/sdX1  (FAT32, BOOT)
├── start4.elf
├── fixup4.dat
├── bootcode.bin
├── u-boot.bin
├── config.txt
├── Image
├── bcm2711-rpi-4-b.dtb
├── overlays/
│   └── *.dtbo
└── extlinux/
    └── extlinux.conf

/dev/sdX2  (ext4, ROOTFS)
├── bin/ sbin/ lib/ usr/ ...
├── etc/  (fstab, hostname, inittab...)
├── lib/modules/<kernel-version>/
└── ...
```

---

## 8. Tài liệu tham khảo chính thức

- U-Boot Raspberry Pi board doc: `u-boot/doc/board/broadcom/raspberrypi.rst` (repo U-Boot chính thức)
- Raspberry Pi Linux kernel: https://github.com/raspberrypi/linux
- Raspberry Pi firmware (GPU blob): https://github.com/raspberrypi/firmware
- Raspberry Pi EEPROM/bootloader: https://github.com/raspberrypi/rpi-eeprom
- Debian debootstrap: `man debootstrap`

---

## 9. Gợi ý bước tiếp theo

- Đóng gói toàn bộ pipeline trên thành **Yocto layer (`meta-raspberrypi`)** hoặc **Buildroot `BR2_EXTERNAL`** để tái lập build tự động, thay vì chạy tay từng bước.
- Thêm cơ chế **A/B OTA update** (RAUC/Mender) bằng cách nhân đôi phân vùng rootfs.
- Bật **Secure Boot** của Pi4 (ký `boot.img` bằng khóa OTP) nếu cần bảo mật chuỗi boot.
