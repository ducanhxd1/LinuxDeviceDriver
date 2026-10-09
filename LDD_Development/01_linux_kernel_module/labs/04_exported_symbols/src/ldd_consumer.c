#include <linux/init.h>
#include <linux/module.h>

#include "ldd_math.h"

static int left = 20;
static int right = 22;
module_param(left, int, 0444);
module_param(right, int, 0444);

static int __init ldd_consumer_init(void)
{
    pr_info("ldd_consumer: %d + %d = %d\n", left, right,
            ldd_add(left, right));
    return 0;
}

static void __exit ldd_consumer_exit(void)
{
    pr_info("ldd_consumer: exit\n");
}

module_init(ldd_consumer_init);
module_exit(ldd_consumer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LDD Development");
MODULE_DESCRIPTION("Exported symbol consumer");
