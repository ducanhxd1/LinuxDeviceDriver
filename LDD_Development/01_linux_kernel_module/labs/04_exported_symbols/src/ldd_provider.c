#include <linux/init.h>
#include <linux/module.h>

#include "ldd_math.h"

int ldd_add(int left, int right)
{
    return left + right;
}
EXPORT_SYMBOL_GPL(ldd_add);

static int __init ldd_provider_init(void)
{
    pr_info("ldd_provider: symbol ldd_add available\n");
    return 0;
}

static void __exit ldd_provider_exit(void)
{
    pr_info("ldd_provider: exit\n");
}

module_init(ldd_provider_init);
module_exit(ldd_provider_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LDD Development");
MODULE_DESCRIPTION("Exported symbol provider");
