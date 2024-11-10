#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched/task.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define KERNEL_SPACE
#include "../comm/comm.c"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Enoz");
MODULE_DESCRIPTION("Kaylee mem");

// Device variables
static int major;
static struct cdev my_cdev;

static int ReadProcessMemory(struct T_RPM args) {
    pid_t reader_pid = current->pid;
    struct task_struct *target_task, *reader_task;
    struct mm_struct *target_mm, *reader_mm;
    char *buffer;
    int bytes_read, bytes_written;

    // Locate the target task
    target_task = get_pid_task(find_get_pid(args.target_pid), PIDTYPE_PID);
    if (!target_task) {
        pr_err("<<klimem>> Could not find target task for PID %d\n",
               args.target_pid);
        return -ESRCH;
    }

    target_mm = get_task_mm(target_task);
    if (!target_mm) {
        pr_err("<<klimem>> Could not access memory map for target PID %d\n",
               args.target_pid);
        put_task_struct(target_task);
        return -EFAULT;
    }

    // Locate the reader task
    reader_task = get_pid_task(find_get_pid(reader_pid), PIDTYPE_PID);
    if (!reader_task) {
        pr_err("<<klimem>> Could not find reader task for PID %d\n",
               reader_pid);
        mmput(target_mm);
        put_task_struct(target_task);
        return -ESRCH;
    }

    reader_mm = get_task_mm(reader_task);
    if (!reader_mm) {
        pr_err("<<klimem>> Could not access reader map for target PID %d\n",
               reader_pid);
        mmput(target_mm);
        put_task_struct(target_task);
        put_task_struct(reader_task);
        return -EFAULT;
    }

    // Allocate kernel buffer for reading data
    buffer = kmalloc(args.read_size, GFP_KERNEL);
    if (!buffer) {
        pr_err("<<klimem>> Failed to allocate buffer\n");
        mmput(target_mm);
        mmput(reader_mm);
        put_task_struct(target_task);
        put_task_struct(reader_task);
        return -ENOMEM;
    }

    // Read the memory from the target process
    bytes_read = access_process_vm(target_task, args.target_address, buffer,
                                   args.read_size, FOLL_FORCE);
    if (bytes_read < 0) {
        pr_err("<<klimem>> access_process_vm (read) failed\n");
        kfree(buffer);
        mmput(target_mm);
        mmput(reader_mm);
        put_task_struct(target_task);
        put_task_struct(reader_task);
        return -EFAULT;
    }

    pr_info("<<klimem>> Read %d bytes from process %d at address 0x%lx\n",
            bytes_read, args.target_pid, args.target_address);

    // Write the data to the reader process
    bytes_written = access_process_vm(reader_task, args.buffer_address, buffer,
                                      bytes_read, FOLL_WRITE);
    if (bytes_written < 0) {
        pr_err("<<klimem>> access_process_vm (write) failed\n");
    } else {
        pr_info("<<klimem>> Wrote %d bytes to process %d at address 0x%lx\n",
                bytes_written, reader_pid, args.buffer_address);
    }

    // Cleanup
    kfree(buffer);
    mmput(target_mm);
    mmput(reader_mm);
    put_task_struct(target_task);
    put_task_struct(reader_task);

    return 0;
}

// ioctl handler
static long ioctl_handler(struct file *file, unsigned int cmd,
                          unsigned long arg) {

    struct T_RPM rpm_val;

    switch (cmd) {
    case IOCTL_RPM:
        if (copy_from_user(&rpm_val, (int __user *)arg, sizeof(struct T_RPM))) {
            return -EFAULT;
        }
        printk(KERN_INFO "RPM Receieved (Caller PID %i) (PID %i, target addr "
                         "%lx, size %zu, local buff %lx)",
               current->pid, rpm_val.target_pid, rpm_val.target_address,
               rpm_val.read_size, rpm_val.buffer_address);
        ReadProcessMemory(rpm_val);
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
