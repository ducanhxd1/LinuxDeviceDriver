#include <linux/init.h>
#include <linux/module.h>

#define MODULE_NAME "ldd_params"

static int repeat = 1;
static bool enabled = true;
static char *message = "hello";

module_param(repeat, int, 0444);
MODULE_PARM_DESC(repeat, "Number of messages during init (1..10)");
module_param(enabled, bool, 0444);
MODULE_PARM_DESC(enabled, "Enable greeting output");
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Greeting text");

static int __init ldd_params_init(void)
{
    int index;

    if (repeat < 1 || repeat > 10) {
        pr_err(MODULE_NAME ": repeat must be in range 1..10\n");
        return -EINVAL;
    }
    pr_info(MODULE_NAME ": enabled=%d repeat=%d\n", enabled, repeat);
    if (enabled)
        for (index = 0; index < repeat; ++index)
            pr_info(MODULE_NAME ": message[%d]=%s\n", index, message);
    return 0;
}

static void __exit ldd_params_exit(void)
{
    pr_info(MODULE_NAME ": exit\n");
}

module_init(ldd_params_init);
module_exit(ldd_params_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LDD Development");
MODULE_DESCRIPTION("Validated module parameters training");

