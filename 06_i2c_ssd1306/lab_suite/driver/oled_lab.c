// SPDX-License-Identifier: GPL-2.0
/* Teaching driver, private sysfs ABI. Not a DRM/fbdev replacement. */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include "protocol.h"

#ifdef OLED_CONST_SYSFS
#define OLED_BIN_CONST const
#else
#define OLED_BIN_CONST
#endif

struct oled_lab {
    struct i2c_client *client;
    struct mutex lock;
    u8 frame[OLED_BYTES];
};

static int oled_tx(struct oled_lab *oled, const u8 *buf, int length)
{
    int ret = i2c_master_send(oled->client, buf, length);
    if (ret < 0)
        return ret;
    return ret == length ? 0 : -EIO;
}

static int oled_command(struct oled_lab *oled, u8 command)
{
    u8 buf[] = {0x00, command};
    return oled_tx(oled, buf, sizeof(buf));
}

/* Caller holds lock for the entire frame, not only one I2C message. */
static int oled_frame(struct oled_lab *oled, const u8 *data)
{
    const u8 window[] = {OLED_WINDOW_BYTES};
    u8 packet[17] = {0x40};
    int offset, ret = oled_tx(oled, window, sizeof(window));

    if (ret)
        return ret;
    for (offset = 0; offset < OLED_BYTES; offset += 16) {
        memcpy(packet + 1, data + offset, 16);
        ret = oled_tx(oled, packet, sizeof(packet));
        if (ret)
            return ret;
    }
    return 0;
}

static int oled_initialize(struct oled_lab *oled)
{
    const u8 init[] = {OLED_INIT_BYTES};
    int ret = oled_tx(oled, init, sizeof(init));

    if (ret)
        return ret;
    memset(oled->frame, 0, sizeof(oled->frame));
    ret = oled_frame(oled, oled->frame);
    if (ret)
        return ret;
    return oled_command(oled, 0xAF);
}

static ssize_t command_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
    struct oled_lab *oled = dev_get_drvdata(dev);
    u8 value;
    int ret;

    if (sysfs_streq(buf, "init")) {
        mutex_lock(&oled->lock);
        ret = oled_initialize(oled);
        mutex_unlock(&oled->lock);
        return ret ? ret : (ssize_t)count;
    }
    if (sysfs_streq(buf, "on")) value = 0xAF;
    else if (sysfs_streq(buf, "off")) value = 0xAE;
    else if (sysfs_streq(buf, "all")) value = 0xA5;
    else if (sysfs_streq(buf, "ram")) value = 0xA4;
    else if (sysfs_streq(buf, "invert")) value = 0xA7;
    else if (sysfs_streq(buf, "normal")) value = 0xA6;
    else return -EINVAL;

    mutex_lock(&oled->lock);
    ret = oled_command(oled, value);
    mutex_unlock(&oled->lock);
    return ret ? ret : (ssize_t)count;
}
static DEVICE_ATTR_WO(command);

static ssize_t contrast_store(struct device *dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct oled_lab *oled = dev_get_drvdata(dev);
    u8 payload[] = {0x00, 0x81, 0x00};
    int ret = kstrtou8(buf, 0, &payload[2]);

    if (ret)
        return ret;
    mutex_lock(&oled->lock);
    ret = oled_tx(oled, payload, sizeof(payload));
    mutex_unlock(&oled->lock);
    return ret ? ret : (ssize_t)count;
}
static DEVICE_ATTR_WO(contrast);

static ssize_t frame_write(struct file *file, struct kobject *kobj,
                           OLED_BIN_CONST struct bin_attribute *attr, char *buf,
                           loff_t offset, size_t count)
{
    struct oled_lab *oled = dev_get_drvdata(kobj_to_dev(kobj));
    int ret;

    if (offset != 0 || count != OLED_BYTES)
        return -EINVAL;
    mutex_lock(&oled->lock);
    ret = oled_frame(oled, (const u8 *)buf);
    if (!ret)
        memcpy(oled->frame, buf, OLED_BYTES);
    mutex_unlock(&oled->lock);
    return ret ? ret : (ssize_t)count;
}

/* Unbounded sysfs size lets the callback reject >512, not silently truncate. */
static struct bin_attribute frame_attr = {
    .attr = { .name = "frame", .mode = 0200 },
    .size = 0,
    .write = frame_write,
};
static struct attribute *oled_attrs[] = {
    &dev_attr_command.attr, &dev_attr_contrast.attr, NULL
};
static OLED_BIN_CONST struct bin_attribute *oled_bin_attrs[] = { &frame_attr, NULL };
static const struct attribute_group oled_group = {
    .attrs = oled_attrs,
    .bin_attrs = oled_bin_attrs,
};

static int oled_probe(struct i2c_client *client)
{
    struct oled_lab *oled;
    int ret;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
        return -EOPNOTSUPP;
    oled = devm_kzalloc(&client->dev, sizeof(*oled), GFP_KERNEL);
    if (!oled)
        return -ENOMEM;
    oled->client = client;
    mutex_init(&oled->lock);
    i2c_set_clientdata(client, oled);

    ret = oled_initialize(oled);
    if (ret) {
        oled_command(oled, 0xAE);
        return dev_err_probe(&client->dev, ret, "initialization failed\n");
    }
    /* Expose userspace ABI only after hardware initialization. */
    ret = sysfs_create_group(&client->dev.kobj, &oled_group);
    if (ret) {
        oled_command(oled, 0xAE);
        return ret;
    }
    dev_info(&client->dev, "128x32 lab ready: command and frame sysfs attributes\n");
    return 0;
}

static void oled_remove(struct i2c_client *client)
{
    struct oled_lab *oled = i2c_get_clientdata(client);

    /* Removal drains active sysfs callbacks before devm memory is released. */
    sysfs_remove_group(&client->dev.kobj, &oled_group);
    mutex_lock(&oled->lock);
    if (oled_command(oled, 0xAE))
        dev_warn(&client->dev, "OFF failed during remove\n");
    mutex_unlock(&oled->lock);
}

static void oled_shutdown(struct i2c_client *client)
{
    struct oled_lab *oled = i2c_get_clientdata(client);
    mutex_lock(&oled->lock);
    oled_command(oled, 0xAE);
    mutex_unlock(&oled->lock);
}

static const struct of_device_id oled_of_match[] = {
    { .compatible = "learning,ssd1306-128x32" },
    { }
};
MODULE_DEVICE_TABLE(of, oled_of_match);
static const struct i2c_device_id oled_ids[] = {
    { "oled_lab", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, oled_ids);

static struct i2c_driver oled_driver = {
    .driver = { .name = "oled_lab", .of_match_table = oled_of_match },
    .probe = oled_probe,
    .remove = oled_remove,
    .shutdown = oled_shutdown,
    .id_table = oled_ids,
};
module_i2c_driver(oled_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSD1306 128x32 teaching driver with private sysfs ABI");
