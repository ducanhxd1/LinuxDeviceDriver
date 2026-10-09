#include <asm-generic/errno-base.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>       // Thu vien cho file operations
#include <linux/cdev.h>     // Thu vien cho Chracter device
#include <linux/device.h>
#include <linux/uaccess.h>  // Thu vien cho copy_from_user
#include <linux/io.h>       // Thu vien cho ioremap va iowrite32


// dia chi base cua pi4 (BCM2711)
#define BCM2711_GPIO_BASE  0xFE200000  // tra datasheet de tim dia chi nay
#define GPIO_REG_SIZE      0xB4        // Kich thuoc vung nho thanh ghi GPIO

// Cac thanh ghi su dung cho GPIO 21 va GPIO 20
#define GPFSEL2_OFFSET  0x08    // Thanh ghi chon chuc nang cho chan 20-29
#define GPSET0_OFFSET   0x1C    // thanh ghi muc HIGH
#define GPCLR0_OFFSET   0x28    // thanh ghi muc LOW
#define GPIO_PIN        21      // Chân Output điều khiển LED

// GPIO20 button
#define GPLEV0_OFFSET   0x34    // Thanh ghi doc trang thai
#define GPIO_PIN_BUTTON 20      // Chân Input đọc nút bấm

// Global variables
static dev_t rpi_gpio_dev_num;        // Chua so Major va minor
static struct cdev rpi_gpio_cdev;     // struct character device
static struct class *rpi_gpio_class;  // struct de tao file trong /dev
static void __iomem *rpi_gpio_base;   // con tro luu dia chi ao sau khi map

// Fops
// ham nay duoc goi khi user space mo file /dev/rpi_gpio
static int rpi_gpio_open(struct inode *inode, struct file *file)
{
    pr_info("rpi_gpio: Device file opened!\n");
    return 0;
}

// ham nay duoc goi khi dong file /dev/rpi_gpio
static int rpi_gpio_release(struct inode *inode, struct file *file)
{
    pr_info("rpi_gpio: Device file closed!\n");
    return 0;
}

static ssize_t rpi_gpio_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offs){
    char kernel_buffer[2] = {0};

    if (count == 0) return 0;
    if (count > 2) count = 2;

    // Sao chep an toan du lieu tu User memory sang Kernel memory
    if (copy_from_user(kernel_buffer, user_buffer, count)) {
        return -EFAULT;
    }

    // Phan tich du lieu nhan duoc de dieu khien
    if (kernel_buffer[0] == '1') {
        pr_info("rpi_gpio: Pull GPIO 21 is HIGH\n");
        // Ghi bit 1 dich trai 21 lan vao thanh ghi SET0 de set muc HIGH (bat LED)
        iowrite32((1 << GPIO_PIN), rpi_gpio_base + GPSET0_OFFSET);
    } else if (kernel_buffer[0] == '0') {
        pr_info("rpi_gpio: Pull GPIO 21 is LOW\n"); // Đã sửa log tại đây
        iowrite32((1 << GPIO_PIN), rpi_gpio_base + GPCLR0_OFFSET);
    } else {
        pr_warn("rpi_gpio: Command not valid. Write '1' or '0'.\n");
    }
    return count;
}

// Đã sửa kiểu dữ liệu trả về từ size_t thành ssize_t ở đây
static ssize_t rpi_gpio_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offs) {
    uint32_t reg_val;
    char status_char;

    // Neu bien offset > 0, nghia la da doc xong (tranh cat doc lap vo han)
    if (*offs > 0) return 0;

    // Doc toan bo trang thai cac chan tu thanh ghi GPLEV0
    reg_val = ioread32(rpi_gpio_base + GPLEV0_OFFSET);

    // check xem bit 20 bang 1 / 0
    if (reg_val & (1 << GPIO_PIN_BUTTON)) {
        status_char = '1'; // HIGH
    } else {
        status_char = '0'; // LOW
    }

    // Copy to user lay trang thai tu trong kernel memory sang user memory
    if (copy_to_user(user_buffer, &status_char, 1)) {
        return -EFAULT;
    }

    *offs = *offs + 1; // cap nhat offset de bao da doc xong 1 byte
    return 1;
}

// Lien ket cac ham callback vao struct file_operations
static struct file_operations rpi_gpio_fops = {
    .owner = THIS_MODULE,
    .open = rpi_gpio_open,
    .release = rpi_gpio_release,
    .write = rpi_gpio_write,
    .read = rpi_gpio_read,
};

static int __init rpi_gpio_init(void) {
    uint32_t reg_val;
    pr_info("rpi_gpio: Init driver\n");

    if (alloc_chrdev_region(&rpi_gpio_dev_num, 0, 1, "rpi_gpio_dev") < 0) {
        pr_err("rpi_gpio: Cannot alloc major/minor number\n");
        return -1;
    }

    cdev_init(&rpi_gpio_cdev, &rpi_gpio_fops);
    if (cdev_add(&rpi_gpio_cdev, rpi_gpio_dev_num, 1) < 0) {
        pr_err("rpi_gpio: Cannot add device in system\n");
        goto r_chrdev;
    }

    rpi_gpio_class = class_create("rpi_gpio_class");
    if (IS_ERR(rpi_gpio_class)) {
        pr_err("rpi_gpio: Cannot create device class\n");
        goto r_cdev;
    }
    if (IS_ERR(device_create(rpi_gpio_class, NULL, rpi_gpio_dev_num, NULL, "rpi_gpio"))) {
        pr_err("rpi_gpio: Cannot create device node trong /dev\n");
        goto r_class;
    }

    rpi_gpio_base = ioremap(BCM2711_GPIO_BASE, GPIO_REG_SIZE);
    if (!rpi_gpio_base) {
        pr_err("rpi_gpio: ioremap physical address failed\n");
        goto r_device;
    }

    // Đọc giá trị hiện tại của thanh ghi GPFSEL2
    reg_val = ioread32(rpi_gpio_base + GPFSEL2_OFFSET);

    // 1. Cấu hình rõ ràng cho chân GPIO 20 làm INPUT (Xóa 3 bit 0,1,2 về 000)
    reg_val &= ~(7 << 0);

    // 2. Cấu hình cho chân GPIO 21 làm OUTPUT
    reg_val &= ~(7 << 3);  // Clear 3 bit cau hinh gpio21 (bit 3,4,5)
    reg_val |= (1 << 3);   // Set che do Output (001)

    // Ghi gia tri moi nguoc lai vao thanh ghi cau hinh
    iowrite32(reg_val, rpi_gpio_base + GPFSEL2_OFFSET);

    pr_info("rpi_gpio: Initialize succeeded! Check in /dev/rpi_gpio\n");
    return 0;

r_device:
    device_destroy(rpi_gpio_class, rpi_gpio_dev_num);
r_class:
    class_destroy(rpi_gpio_class);
r_cdev:
    cdev_del(&rpi_gpio_cdev);
r_chrdev:
    unregister_chrdev_region(rpi_gpio_dev_num, 1);
    return -1;
}

static void __exit rpi_gpio_exit(void) {
    pr_info("rpi_gpio: exit driver\n");

    if (rpi_gpio_base) {
        iounmap(rpi_gpio_base);
    }

    device_destroy(rpi_gpio_class, rpi_gpio_dev_num);
    class_destroy(rpi_gpio_class);
    cdev_del(&rpi_gpio_cdev);
    unregister_chrdev_region(rpi_gpio_dev_num, 1);

    pr_info("rpi_gpio: exit succeeded!\n");
}

module_init(rpi_gpio_init);
module_exit(rpi_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DUC ANH");
MODULE_DESCRIPTION("GPIO example driver");

