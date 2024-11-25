#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

static int __init usb_ipc_init(void) {
    printk(KERN_EMERG "Test init!\n");
    return 0;
}

static void __exit usb_ipc_exit(void) {
    printk(KERN_EMERG "Test remove!\n");
}

module_init(usb_ipc_init);
module_exit(usb_ipc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmytro Stechenko <dmytro.stechenko@gmail.com>");