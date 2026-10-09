#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#define MODULE_NAME "ldd_memory"
#define PROC_NAME "ldd_memory"

static unsigned int buffer_size = 128;
static char *buffer;
static struct proc_dir_entry *proc_entry;

module_param(buffer_size, uint, 0444);
MODULE_PARM_DESC(buffer_size, "Allocated bytes (16..4096)");

static int memory_show(struct seq_file *sequence, void *unused)
{
    (void)unused;
    seq_printf(sequence, "buffer_size=%u\n", buffer_size);
    seq_printf(sequence, "message=%s\n", buffer);
    return 0;
}

static int memory_open(struct inode *inode, struct file *file)
{
    (void)inode;
    return single_open(file, memory_show, NULL);
}

static const struct proc_ops memory_proc_ops = {
    .proc_open = memory_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init ldd_memory_init(void)
{
    if (buffer_size < 16 || buffer_size > 4096)
        return -EINVAL;

    buffer = kzalloc(buffer_size, GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;

    strscpy(buffer, "allocated kernel memory is alive", buffer_size);
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &memory_proc_ops);
    if (!proc_entry) {
        kfree(buffer);
        buffer = NULL;
        return -ENOMEM;
    }

    pr_info(MODULE_NAME ": allocated %u bytes and created /proc/%s\n",
            buffer_size, PROC_NAME);
    return 0;
}

static void __exit ldd_memory_exit(void)
{
    proc_remove(proc_entry);
    kfree(buffer);
    pr_info(MODULE_NAME ": proc entry removed and memory freed\n");
}

module_init(ldd_memory_init);
module_exit(ldd_memory_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LDD Development");
MODULE_DESCRIPTION("Kernel allocation and procfs lifecycle training");

