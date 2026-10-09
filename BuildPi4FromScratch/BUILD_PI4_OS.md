# Build a bootable Linux OS for Raspberry Pi 4 from scratch

This guide builds a small, reproducible 64-bit OS image for Raspberry Pi 4 Model B from an x86_64 Debian or Ubuntu host:

```text
[1. Toolchain] -> [2. Raspberry Pi firmware / optional U-Boot]
                -> [3. Linux kernel]
                -> [4. Debian rootfs]
                -> [5. SD card partitions]
                -> boot Raspberry Pi 4
```

The default path uses the Raspberry Pi EEPROM bootloader and Raspberry Pi GPU firmware to load the Linux kernel directly. U-Boot is included as an optional second-stage bootloader. Direct firmware boot has fewer moving parts and is the recommended first successful boot.

## 0. Scope and boot model

Target hardware:

- Raspberry Pi 4 Model B / BCM2711, 64-bit (`aarch64`)
- microSD card of at least 8 GB; 16 GB or larger is more comfortable
- 5.1 V / 3 A USB-C power supply recommended for Pi 4
- USB-TTL 3.3 V serial adapter is strongly recommended for debugging
- Linux host with Internet access and administrator privileges

Boot chain for the default profile:

```text
Pi 4 ROM
  -> SPI EEPROM bootloader
  -> FAT32 boot partition
  -> start4.elf + fixup4.dat + config.txt
  -> bcm2711-rpi-4-b.dtb
  -> kernel8.img
  -> Linux kernel
  -> /sbin/init in Debian rootfs
```

Important Pi 4 facts:

- Pi 4 uses a bootloader stored in SPI EEPROM. `bootcode.bin` is not needed.
- The first partition must be FAT16/FAT32 and contain the Raspberry Pi firmware files.
- `config.txt` and `cmdline.txt` are still used by the Pi 4 firmware.
- A 64-bit Pi 4 kernel is normally named `kernel8.img` when loaded directly by the firmware.
- Device Tree is required by modern Raspberry Pi Linux kernels. Do not disable it.

This guide does not create a production distribution. It creates a minimal engineering OS suitable for learning, driver development, serial-console boot, networking, and further customization.

> Safety: every command that writes a block device is marked with `SD_DEVICE`. Verify it with `lsblk` before running it. A wrong value will destroy another disk.

## 1. Host packages and toolchain

### 1.1 Install required packages

On Debian or Ubuntu x86_64:

```bash
sudo apt update
sudo apt install -y \
  bc bison build-essential bzip2 cpio debootstrap \
  device-tree-compiler dosfstools e2fsprogs flex \
  git kmod libssl-dev parted pkg-config rsync \
  u-boot-tools qemu-user-static binfmt-support \
  gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu \
  wget xz-utils ca-certificates
```

The package names above are host-package names. On a different distribution, install equivalent packages. The kernel build system itself documents minimum tool versions in `Documentation/process/changes.rst`; if a build fails with a version error, satisfy that document first rather than forcing the build.

Check the cross compiler:

```bash
aarch64-linux-gnu-gcc --version
aarch64-linux-gnu-ld --version
update-binfmts --display qemu-aarch64 2>/dev/null || true
```

The compiler target must be AArch64. Do not use `arm-linux-gnueabihf-` for this 64-bit build.

### 1.2 Define a workspace and versions

Use a clean build directory outside the kernel source tree:

```bash
export WORK="$HOME/pi4-os-build"
export SRC="$WORK/src"
export OUT="$WORK/out"
export BOOT="$OUT/boot"
export ROOTFS="$OUT/rootfs"
export JOBS="$(nproc)"

mkdir -p "$SRC" "$BOOT" "$ROOTFS"
```

Pin source revisions when building an image that must be reproducible. The following commands use named upstream branches/tags as examples; record the resulting Git commit IDs in your build notes:

```bash
export FIRMWARE_REF="master"
export UBOOT_REF="master"
export KERNEL_REF="v6.x"       # replace with a released stable tag, e.g. v6.12.x
export DEBIAN_SUITE="trixie"   # change only when intentionally targeting another suite
export DEBIAN_MIRROR="https://deb.debian.org/debian"
```

For a long-lived product, replace moving names such as `master`, `v6.x`, and `trixie` with exact commits, tags, or snapshot URLs and save SHA-256 checksums of downloaded artifacts.

## 2. Raspberry Pi firmware and optional U-Boot

There are two valid configurations.

### 2.1 Recommended first boot: Raspberry Pi firmware loads Linux directly

Get the official firmware repository:

```bash
cd "$SRC"
git clone --depth=1 https://github.com/raspberrypi/firmware.git
cd firmware
git rev-parse HEAD
```

Copy the Pi 4 firmware and supporting files into the boot staging directory:

```bash
rsync -a --delete \
  boot/ "$BOOT/"
```

The firmware repository contains more files than strictly necessary, which is useful during bring-up. At minimum, Pi 4 needs the matching `start4.elf` and `fixup4.dat`, the Pi 4 DTB, and the `overlays/` directory. Keeping the repository's complete `boot/` contents avoids accidentally omitting a firmware dependency.

The Pi 4 EEPROM bootloader normally already supports SD boot. If the board is old or behaves unexpectedly, boot the board once with the official Raspberry Pi Imager `Misc utility images -> Bootloader -> SD Card Boot`, or update it from Raspberry Pi OS with:

```bash
sudo apt update
sudo apt full-upgrade
sudo rpi-eeprom-update
sudo rpi-eeprom-update -a
sudo reboot
```

Do not update the EEPROM by copying random files from an unrelated firmware version. The EEPROM image must be appropriate for BCM2711 and the board's minimum manufacturing version.

### 2.2 Optional U-Boot profile

Use U-Boot when you need a boot menu, multiple kernels, scripted recovery, network boot, or a common boot flow across boards. It is not needed merely to boot Linux on a Pi 4.

Build U-Boot for Pi 4:

```bash
cd "$SRC"
git clone --depth=1 https://source.denx.de/u-boot/u-boot.git
cd u-boot
git rev-parse HEAD
make distclean
make rpi_4_defconfig
make -j"$JOBS" CROSS_COMPILE=aarch64-linux-gnu-
cp u-boot.bin "$BOOT/u-boot.bin"
```

For this U-Boot profile, the Pi firmware loads `u-boot.bin` as its kernel. Create `config.txt` later with:

```ini
[all]
arm_64bit=1
kernel=u-boot.bin
enable_uart=1
```

Use a U-Boot script only if you understand its storage addressing and boot environment. A more maintainable approach is U-Boot's `extlinux` support. Create `$BOOT/extlinux/extlinux.conf` after the kernel and rootfs are installed:

```text
DEFAULT linux
TIMEOUT 20

LABEL linux
    LINUX /kernel8.img
    FDT /bcm2711-rpi-4-b.dtb
    APPEND root=PARTUUID=<ROOT_PARTUUID> rootfstype=ext4 rw rootwait console=serial0,115200 console=tty1
```

Replace `<ROOT_PARTUUID>` with the actual PARTUUID of the second partition. U-Boot must be able to read the filesystem containing `extlinux.conf`; keeping the boot files on the FAT partition is the simplest Pi 4 arrangement.

For the initial bring-up, choose exactly one mode:

- Direct mode: do not copy `u-boot.bin`; set `kernel=kernel8.img` or omit `kernel=`.
- U-Boot mode: copy `u-boot.bin`; set `kernel=u-boot.bin`; use `extlinux.conf`.

Do not leave both modes ambiguous while debugging.

## 3. Build the Linux kernel

### 3.1 Fetch and verify the kernel source

Use the official Linux repository or a Raspberry Pi kernel tree. Mainline Linux is preferred for a portable, upstream-oriented system; the Raspberry Pi tree can contain additional Pi-specific fixes and drivers.

```bash
cd "$SRC"
git clone --depth=1 --branch "$KERNEL_REF" \
  https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git linux
cd linux
git rev-parse HEAD
```

If the chosen stable tag is not available in the shallow clone, clone the full repository or use a released tarball from `https://www.kernel.org/` and verify its signature/checksum.

### 3.2 Configure a 64-bit BCM2711 kernel

Use an out-of-tree build directory. Every later `make` invocation must keep the same `O=` value.

```bash
export KBUILD="$OUT/kernel-build"
mkdir -p "$KBUILD"

cd "$SRC/linux"
make O="$KBUILD" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
```

Enable the features required for a root filesystem on the second SD partition. They are deliberately built into the kernel (`=y`) so an initramfs is not required for this first boot:

```bash
cd "$SRC/linux"
scripts/config --file "$KBUILD/.config" \
  --enable CONFIG_64BIT \
  --enable CONFIG_ARCH_BCM2835 \
  --enable CONFIG_ARCH_BCM2711 \
  --enable CONFIG_OF \
  --enable CONFIG_DEVTMPFS \
  --enable CONFIG_DEVTMPFS_MOUNT \
  --enable CONFIG_SERIAL_AMBA_PL003 \
  --enable CONFIG_SERIAL_AMBA_PL011 \
  --enable CONFIG_SERIAL_AMBA_PL011_CONSOLE \
  --enable CONFIG_SERIAL_8250 \
  --enable CONFIG_SERIAL_8250_CONSOLE \
  --enable CONFIG_MMC \
  --enable CONFIG_MMC_BLOCK \
  --enable CONFIG_MMC_BCM2835 \
  --enable CONFIG_MMC_SDHCI \
  --enable CONFIG_MMC_SDHCI_IPROC \
  --enable CONFIG_EXT4_FS \
  --enable CONFIG_EXT4_USE_FOR_EXT2 \
  --enable CONFIG_VFAT_FS \
  --enable CONFIG_NLS_CODEPAGE_437 \
  --enable CONFIG_NLS_ISO8859_1 \
  --enable CONFIG_NET \
  --enable CONFIG_INET \
  --enable CONFIG_IP_PNP \
  --enable CONFIG_IP_PNP_DHCP \
  --enable CONFIG_USB_XHCI_HCD \
  --enable CONFIG_USB_XHCI_PLATFORM \
  --enable CONFIG_USB_STORAGE \
  --enable CONFIG_USB_DWC2 \
  --enable CONFIG_FRAMEBUFFER_CONSOLE

make O="$KBUILD" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
```

Some kernel versions rename or reorganize individual symbols. If `scripts/config` reports an unknown symbol, inspect the current configuration with `make O="$KBUILD" ARCH=arm64 menuconfig`, search for the corresponding feature, then verify the final values:

```bash
grep -E 'CONFIG_(ARCH_BCM|OF=|DEVTMPFS|MMC|EXT4_FS|VFAT_FS|SERIAL|USB|INET|IP_PNP)' "$KBUILD/.config"
```

For a production kernel, review the complete configuration instead of copying this fragment blindly. Enable the driver for every storage, network, display, Wi-Fi, Bluetooth, GPIO, I2C, SPI, and USB device that your product needs. Build optional drivers as modules and install them into the rootfs.

### 3.3 Compile the image, DTBs, and modules

```bash
cd "$SRC/linux"
make O="$KBUILD" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  -j"$JOBS" Image.gz dtbs modules
```

The expected artifacts include:

```text
$KBUILD/arch/arm64/boot/Image.gz
$KBUILD/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb
```

Install them into the boot staging directory. The Pi firmware can load a gzip-compressed kernel image directly:

```bash
cp "$KBUILD/arch/arm64/boot/Image.gz" "$BOOT/kernel8.img"
cp "$KBUILD/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb" \
   "$BOOT/bcm2711-rpi-4-b.dtb"

sudo make -C "$SRC/linux" O="$KBUILD" ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH="$ROOTFS" modules_install
```

If the kernel build produces a different DTB path, locate it with:

```bash
find "$KBUILD/arch/arm64/boot/dts" -name 'bcm2711-rpi-4-b.dtb' -print
```

Do not copy a DTB from a different kernel build into the boot partition. Kernel and DTB should come from the same source revision.

## 4. Build the Debian root filesystem

### 4.1 Bootstrap a minimal arm64 Debian rootfs

`debootstrap --foreign` performs the unpacking stage on an x86_64 host. The second stage must run as AArch64; `qemu-aarch64-static` enables that on the host.

```bash
sudo rm -rf "$ROOTFS"
sudo mkdir -p "$ROOTFS"

sudo debootstrap --arch=arm64 --foreign --variant=minbase \
  "$DEBIAN_SUITE" "$ROOTFS" "$DEBIAN_MIRROR"

sudo cp /usr/bin/qemu-aarch64-static "$ROOTFS/usr/bin/"
sudo chroot "$ROOTFS" /debootstrap/debootstrap --second-stage
```

`debootstrap` checks Debian Release signatures when the required keyring is available. Do not use `--no-check-gpg` for a normal build.

### 4.2 Configure the target rootfs

Create a resolvable DNS configuration for package installation and mount virtual filesystems while entering the chroot:

```bash
sudo cp -L /etc/resolv.conf "$ROOTFS/etc/resolv.conf"
sudo mount --bind /dev "$ROOTFS/dev"
sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
sudo mount -t proc proc "$ROOTFS/proc"
sudo mount -t sysfs sys "$ROOTFS/sys"
sudo mount -t tmpfs tmpfs "$ROOTFS/run"
```

Set the hostname, repositories, and a serial-console login:

```bash
sudo tee "$ROOTFS/etc/hostname" >/dev/null <<'EOF'
pi4
EOF

sudo tee "$ROOTFS/etc/hosts" >/dev/null <<'EOF'
127.0.0.1 localhost
127.0.1.1 pi4
::1 localhost ip6-localhost ip6-loopback
EOF

sudo tee "$ROOTFS/etc/apt/sources.list.d/debian.sources" >/dev/null <<EOF
Types: deb
URIs: $DEBIAN_MIRROR
Suites: $DEBIAN_SUITE $DEBIAN_SUITE-updates
Components: main
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg

Types: deb
URIs: http://security.debian.org/debian-security
Suites: $DEBIAN_SUITE-security
Components: main
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg
EOF
```

Install the minimum useful userspace. `systemd-sysv` supplies `/sbin/init`; `ifupdown` gives a small traditional Ethernet setup. Add NetworkManager later if the image needs modern Wi-Fi provisioning.

```bash
sudo chroot "$ROOTFS" /bin/bash <<'EOF'
set -e
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  systemd-sysv udev sudo openssh-server \
  iproute2 iputils-ping ifupdown isc-dhcp-client \
  ca-certificates kmod locales less vim-tiny

echo 'en_US.UTF-8 UTF-8' > /etc/locale.gen
locale-gen
dpkg-reconfigure -f noninteractive locales
ln -sf /usr/share/zoneinfo/UTC /etc/localtime

echo 'UTC' > /etc/timezone

passwd --delete root
useradd -m -s /bin/bash pi
usermod -aG sudo,dialout,video,input,plugdev pi
passwd pi

cat > /etc/network/interfaces <<'NET'
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
NET

systemctl enable ssh
systemctl enable systemd-networkd 2>/dev/null || true

apt-get clean
rm -rf /var/lib/apt/lists/*
EOF
```

The `passwd pi` command is interactive. For unattended image creation, use a locked account and install an SSH key instead; never commit a clear-text password into this guide or an image build script.

If the kernel uses modules, generate dependency metadata after the kernel version is known:

```bash
sudo chroot "$ROOTFS" depmod -a
```

### 4.3 Add an early boot test service

This makes the first successful userspace boot obvious on the serial console:

```bash
sudo tee "$ROOTFS/etc/systemd/system/first-boot-message.service" >/dev/null <<'EOF'
[Unit]
Description=Pi 4 first boot message
After=multi-user.target

[Service]
Type=oneshot
ExecStart=/usr/bin/logger -t first-boot "Pi 4 custom Debian rootfs is running"
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

sudo chroot "$ROOTFS" systemctl enable first-boot-message.service
```

### 4.4 Unmount the rootfs build environment

```bash
sudo umount -R "$ROOTFS/dev" 2>/dev/null || true
sudo umount "$ROOTFS/proc" 2>/dev/null || true
sudo umount "$ROOTFS/sys" 2>/dev/null || true
sudo umount "$ROOTFS/run" 2>/dev/null || true
```

Before packaging, remove host-only emulation if it will not be needed on the target:

```bash
sudo rm -f "$ROOTFS/usr/bin/qemu-aarch64-static"
```

## 5. Partition and populate the SD card

### 5.1 Recommended layout

Use an MBR partition table for maximum compatibility:

| Partition | Size | Type | Filesystem | Purpose |
|---|---:|---|---|---|
| 1 | 512 MiB | W95 FAT32 LBA | FAT32 | Pi firmware, DTB, kernel, optional U-Boot |
| 2 | Remaining space | Linux | ext4 | Debian rootfs |

The first partition is deliberately larger than the minimum so it can hold multiple kernels, initramfs files, overlays, and recovery files later.

### 5.2 Identify the SD card

Insert the card and inspect devices:

```bash
lsblk -dpno NAME,SIZE,MODEL,TRAN
```

Set the whole-disk device, never a partition:

```bash
export SD_DEVICE=/dev/sdX
# Examples: /dev/sdb or /dev/mmcblk0
lsblk "$SD_DEVICE"
```

Stop here and verify `SD_DEVICE`. The following commands erase it.

### 5.3 Create partitions and filesystems

```bash
sudo umount "${SD_DEVICE}"?* 2>/dev/null || true
sudo wipefs --all "$SD_DEVICE"
sudo parted --script "$SD_DEVICE" \
  mklabel msdos \
  mkpart primary fat32 1MiB 513MiB \
  set 1 boot on \
  mkpart primary ext4 513MiB 100%

sudo partprobe "$SD_DEVICE"
```

Select partition names correctly. `mmcblk0` and NVMe devices use `p1`/`p2`; ordinary SCSI disks use `1`/`2`:

```bash
if [[ "$SD_DEVICE" == /dev/mmcblk* || "$SD_DEVICE" == /dev/nvme* ]]; then
  export SD_BOOT="${SD_DEVICE}p1"
  export SD_ROOT="${SD_DEVICE}p2"
else
  export SD_BOOT="${SD_DEVICE}1"
  export SD_ROOT="${SD_DEVICE}2"
fi

sudo mkfs.vfat -F 32 -n PI_BOOT "$SD_BOOT"
sudo mkfs.ext4 -L PI_ROOT "$SD_ROOT"
```

Record stable identifiers for the kernel command line:

```bash
sudo blkid "$SD_BOOT" "$SD_ROOT"
export ROOT_PARTUUID="$(sudo blkid -s PARTUUID -o value "$SD_ROOT")"
export BOOT_PARTUUID="$(sudo blkid -s PARTUUID -o value "$SD_BOOT")"
echo "ROOT_PARTUUID=$ROOT_PARTUUID"
echo "BOOT_PARTUUID=$BOOT_PARTUUID"
```

### 5.4 Copy the boot files and rootfs

```bash
export MNT_BOOT="$WORK/mnt-boot"
export MNT_ROOT="$WORK/mnt-root"
sudo mkdir -p "$MNT_BOOT" "$MNT_ROOT"

sudo mount "$SD_BOOT" "$MNT_BOOT"
sudo mount "$SD_ROOT" "$MNT_ROOT"

sudo rsync -aHAX "$ROOTFS/" "$MNT_ROOT/"
sudo rsync -a "$BOOT/" "$MNT_BOOT/"
```

Create the firmware configuration for direct boot:

```bash
sudo tee "$MNT_BOOT/config.txt" >/dev/null <<'EOF'
[all]
arm_64bit=1
enable_uart=1
# Keep the default Device Tree selection for Raspberry Pi 4.
# Add only the overlays and dtparams required by the hardware.
EOF
```

Create `cmdline.txt`. It must be one physical line:

```bash
sudo tee "$MNT_BOOT/cmdline.txt" >/dev/null <<EOF
console=serial0,115200 console=tty1 root=PARTUUID=$ROOT_PARTUUID rootfstype=ext4 rootwait rw fsck.repair=yes
EOF
```

The `console=serial0,115200` setting uses GPIO14/GPIO15 on Pi 4. Connect a 3.3 V USB-TTL adapter as follows:

| Pi header | Function | Adapter |
|---:|---|---|
| 6 | GND | GND |
| 8 | GPIO14 TXD | adapter RXD |
| 10 | GPIO15 RXD | adapter TXD |

Use 115200 baud, 8 data bits, no parity, 1 stop bit. Never connect a 5 V UART signal to the Pi.

For U-Boot mode, replace the direct-mode files with:

```bash
sudo tee "$MNT_BOOT/config.txt" >/dev/null <<'EOF'
[all]
arm_64bit=1
kernel=u-boot.bin
enable_uart=1
EOF
sudo mkdir -p "$MNT_BOOT/extlinux"
sudo tee "$MNT_BOOT/extlinux/extlinux.conf" >/dev/null <<EOF
DEFAULT linux
TIMEOUT 20

LABEL linux
    LINUX /kernel8.img
    FDT /bcm2711-rpi-4-b.dtb
    APPEND console=serial0,115200 console=tty1 root=PARTUUID=$ROOT_PARTUUID rootfstype=ext4 rootwait rw fsck.repair=yes
EOF
```

When U-Boot is selected, confirm that `$MNT_BOOT/u-boot.bin` exists. When direct mode is selected, confirm that `$MNT_BOOT/kernel8.img` exists and no `kernel=u-boot.bin` line remains.

### 5.5 Configure fstab and unmount

Use filesystem identifiers so the root partition can move to another device without changing the kernel command line:

```bash
export ROOT_UUID="$(sudo blkid -s UUID -o value "$SD_ROOT")"
export BOOT_UUID="$(sudo blkid -s UUID -o value "$SD_BOOT")"

sudo tee "$MNT_ROOT/etc/fstab" >/dev/null <<EOF
UUID=$ROOT_UUID / ext4 defaults,noatime 0 1
UUID=$BOOT_UUID /boot/firmware vfat defaults 0 2
EOF

sudo mkdir -p "$MNT_ROOT/boot/firmware"
sudo sync
sudo umount "$MNT_ROOT"
sudo umount "$MNT_BOOT"
sudo rmdir "$MNT_ROOT" "$MNT_BOOT"
```

Flush the host's write cache before removing the card:

```bash
sudo sync
sudo eject "$SD_DEVICE" 2>/dev/null || true
```

## 6. First boot and validation

1. Insert the microSD card into the Pi 4.
2. Connect Ethernet if using DHCP.
3. Connect the serial adapter if available.
4. Connect HDMI if you want firmware diagnostics.
5. Apply power using the recommended USB-C supply.

Expected serial milestones:

```text
U-Boot ...                         # only in U-Boot mode
Booting Linux on physical CPU ...
Run /init as init process
systemd[1]: Startup finished ...
login:                            # Debian userspace
```

After login, verify:

```bash
uname -a
cat /proc/device-tree/model; echo
cat /proc/cmdline
findmnt /
findmnt /boot/firmware
ip link
ip addr
systemctl --failed
journalctl -b -p warning
```

Expected values include:

- `aarch64` or `arm64` kernel architecture
- model containing `Raspberry Pi 4`
- root mounted as ext4 from the second SD partition
- a valid `/proc/device-tree` tree
- Ethernet interface receiving a DHCP address when a network is connected

If the kernel starts but `/sbin/init` fails, the firmware and kernel stages are working; debug the rootfs, `fstab`, dynamic libraries, or init system. If there is no kernel output at all, debug the FAT partition, firmware files, EEPROM boot order, kernel filename, DTB, and power supply first.

## 7. Checks before declaring the image bootable

On the host, inspect the finished card:

```bash
sudo fsck.fat -vn "$SD_BOOT"
sudo e2fsck -fn "$SD_ROOT"
```

Mount it again read-only and check the boot contract:

```bash
sudo mount -o ro "$SD_BOOT" "$MNT_BOOT"
ls -lh "$MNT_BOOT/start4.elf" "$MNT_BOOT/fixup4.dat" \
      "$MNT_BOOT/bcm2711-rpi-4-b.dtb" "$MNT_BOOT/kernel8.img"
awk 'NF != 0 { print length($0), $0 }' "$MNT_BOOT/cmdline.txt"
sudo umount "$MNT_BOOT"
```

For U-Boot mode, check instead:

```bash
ls -lh "$MNT_BOOT/u-boot.bin" "$MNT_BOOT/extlinux/extlinux.conf"
```

Keep a checksum manifest for every boot-critical file:

```bash
sha256sum "$BOOT/kernel8.img" \
          "$BOOT/bcm2711-rpi-4-b.dtb" \
          "$BOOT/start4.elf" \
          "$BOOT/fixup4.dat" > "$WORK/boot.sha256"
```

## 8. Troubleshooting matrix

### No green activity LED or no HDMI diagnostics

- Verify the card is actually detected by the host and the FAT partition is first.
- Verify the Pi 4 EEPROM bootloader is not damaged or configured for an unexpected boot order.
- Reflash the EEPROM with Raspberry Pi Imager's Pi 4 SD Card Boot recovery utility.
- Test with a known-good power supply and microSD card.
- Check the Pi 4 activity LED flash code and HDMI boot diagnostics.

### Firmware starts but says kernel not found

- Direct mode requires `kernel8.img` in the FAT partition.
- Confirm `config.txt` does not point to a missing filename.
- Confirm the kernel is an AArch64 `Image` or `Image.gz`, not an x86 image or a 32-bit `zImage`.
- Confirm the DTB matches the kernel source and is named `bcm2711-rpi-4-b.dtb`.

### Kernel panic: cannot mount root fs

- Check `root=PARTUUID=...` against `sudo blkid "$SD_ROOT"`.
- Check `CONFIG_MMC`, the Pi SDHCI driver, `CONFIG_EXT4_FS`, and `CONFIG_DEVTMPFS_MOUNT` are built in, not modules.
- Check `rootwait` is present.
- Confirm the root partition is ext4 and passes `e2fsck`.

### Kernel starts but no serial output

- Use a 3.3 V adapter and cross TX/RX correctly.
- Use 115200 8N1.
- Check `enable_uart=1` and `console=serial0,115200`.
- On Pi 4, use GPIO14/GPIO15, physical pins 8/10.
- If using U-Boot, check U-Boot's console output before Linux's output.

### Login fails or systemd cannot start

- Ensure `/sbin/init` exists and points to a valid systemd binary.
- Ensure `/etc/fstab` has the correct root and boot UUIDs.
- Do not leave host `/etc/resolv.conf` as a dangling symlink in a chroot image.
- Run `systemctl --failed` and `journalctl -b` from the serial console.

### Ethernet is missing

- Enable the BCM2711 Ethernet driver in the kernel configuration.
- Check the Device Tree was loaded and is compatible with the kernel.
- For a minimal system, configure `eth0` through `ifupdown` as shown above, or install NetworkManager/systemd-networkd intentionally.
- Test with a known-good cable and DHCP server.

## 9. Updating the build

For each update, treat these as a compatibility set:

1. firmware repository revision
2. kernel source revision and `.config`
3. DTBs and overlays from the same kernel/firmware plan
4. Debian suite and package snapshot
5. bootloader EEPROM version/configuration

Build a new card or keep a known-good kernel as `kernel8-old.img`. Never overwrite the only working kernel while debugging. Use exact Git commit IDs and package snapshots for a release image, and test power-loss recovery before deploying to unattended hardware.

## 10. References

- Raspberry Pi documentation: https://www.raspberrypi.com/documentation/computers/raspberry-pi.html
- Raspberry Pi configuration and firmware: https://www.raspberrypi.com/documentation/computers/configuration.html
- Raspberry Pi firmware repository: https://github.com/raspberrypi/firmware
- Raspberry Pi Linux repository: https://github.com/raspberrypi/linux
- U-Boot Raspberry Pi board documentation: https://docs.u-boot.org/en/latest/board/broadcom/raspberrypi.html
- U-Boot source: https://source.denx.de/u-boot/u-boot
- Linux kernel build documentation: https://www.kernel.org/doc/html/latest/admin-guide/README.html
- Debian debootstrap manual: https://manpages.debian.org/bookworm/debootstrap/debootstrap.8.en.html
- Debian archive and release information: https://www.debian.org/releases/

This document was written against the Raspberry Pi EEPROM boot flow and current Linux/Debian tooling available at the time of writing. Re-check the linked upstream documentation and record exact versions before using it for a production image.
