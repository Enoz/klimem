#include "asm/current.h"
#include <linux/cdev.h> // For character device
#include <linux/fs.h>   // For file operations structure
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h> // For copy_to_user, copy_from_user

#define KERNEL_SPACE
#include "../comm/comm.c"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Enoz");
MODULE_DESCRIPTION("Kaylee mem");

// Device variables
static int major;
static struct cdev my_cdev;

// ioctl handler
static long ioctl_handler(struct file *file, unsigned int cmd,
                          unsigned long arg) {

    struct T_RPM rpmVal;
    pid_t callerPid;

    switch (cmd) {
    case IOCTL_RPM:
        if (copy_from_user(&rpmVal, (int __user *)arg, sizeof(struct T_RPM))) {
            return -EFAULT;
        }
        callerPid = current->pid;
        printk(KERN_INFO "RPM Receieved from PID %i, %d, %d\n", callerPid,
               rpmVal.tVal1, rpmVal.tVal2);
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

// File operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = ioctl_handler,
};

static int __init my_module_init(void) {
    dev_t dev;
    int ret;

    // Allocate major number
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "<klimem> Failed to allocate major number\n");
        return ret;
    }
    major = MAJOR(dev);

    // Initialize character device
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    // Add character device to the system
    ret = cdev_add(&my_cdev, dev, 1);
    if (ret) {
        printk(KERN_ALERT "<klimem> Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    printk(KERN_INFO "<klimem> Module initialized with major number %d\n",
           major);
    return 0;
}

static void __exit my_module_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(MKDEV(major, 0), 1);
    printk(KERN_INFO "<klimem> Module exited\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
