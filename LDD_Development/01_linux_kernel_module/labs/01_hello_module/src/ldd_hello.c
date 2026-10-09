#include <linux/init.h>
#include <linux/module.h>

#define MODULE_NAME "ldd_hello"

static int __init ldd_hello_init(void)
{
    pr_info(MODULE_NAME ": init: hello from kernel space\n");
    return 0;
}

static void __exit ldd_hello_exit(void)
{
    pr_info(MODULE_NAME ": exit: resources released\n");
}

module_init(ldd_hello_init);
module_exit(ldd_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LDD Development");
MODULE_DESCRIPTION("Minimal lifecycle training module");
MODULE_VERSION("1.0");

