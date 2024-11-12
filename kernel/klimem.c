#include "linux/gfp_types.h"
#include "linux/printk.h"
#include "linux/sched/signal.h"
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/maple_tree.h>
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

static void ListModulesForProcess(struct T_MODULE_REQUEST mod_req) {
    struct T_MODULES *mods = kmalloc(sizeof(struct T_MODULES), GFP_KERNEL);
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    unsigned long index = 0; // Start at the beginning of the tree

    // Find the task for the given PID
    task = get_pid_task(find_get_pid(mod_req.pid), PIDTYPE_PID);
    if (!task) {
        pr_err("<<klimem>> Could not find task for PID %d\n", mod_req.pid);
        return;
    }

    mm = get_task_mm(task);
    if (!mm) {
        pr_err("<<klimem>> Could not access memory map for PID %d\n",
               mod_req.pid);
        put_task_struct(task);
        return;
    }

    // Traverse the maple_tree for VMAs
    down_read(&mm->mmap_lock);
    while ((vma = mt_find(&mm->mm_mt, &index, ULONG_MAX))) {
        if (vma->vm_file) {
            char *path_buf;
            char *path;

            // Allocate a buffer for the file path
            path_buf = kmalloc(PATH_MAX, GFP_KERNEL);
            if (!path_buf) {
                pr_err(
                    "<<klimem>> Failed to allocate memory for path buffer\n");
                up_read(&mm->mmap_lock);
                mmput(mm);
                put_task_struct(task);
                kfree(mods);
                return;
            }

            // Get the file path of the mapped region
            path = d_path(&vma->vm_file->f_path, path_buf, PATH_MAX);
            if (IS_ERR(path)) {
                pr_err("<<klimem>> Failed to get path for module\n");
                kfree(path_buf);
                continue;
            }
            mods->modules[mods->numModules].start = vma->vm_start;
            mods->modules[mods->numModules].end = vma->vm_end;
            strncpy(mods->modules[mods->numModules].path, path,
                    sizeof(mods->modules[mods->numModules].path));

            mods->numModules++;
            kfree(path_buf);
        }

        // Move `index` forward to continue traversal
        index = vma->vm_end;
    }
    up_read(&mm->mmap_lock);

    mmput(mm);
    put_task_struct(task);

    // Write to user

    pid_t reader_pid = current->pid;
    struct task_struct *reader_task;
    struct mm_struct *reader_mm;
    // Locate the reader task
    reader_task = get_pid_task(find_get_pid(reader_pid), PIDTYPE_PID);
    if (!reader_task) {
        pr_err("<<klimem>> Could not find reader task for PID %d\n",
               reader_pid);
        kfree(mods);
        return;
    }

    reader_mm = get_task_mm(reader_task);
    if (!reader_mm) {
        pr_err("<<klimem>> Could not access reader map for target PID %d\n",
               reader_pid);
        put_task_struct(reader_task);
        kfree(mods);
        return;
    }

    int bytes_written =
        access_process_vm(reader_task, mod_req.buffer_address, mods,
                          sizeof(struct T_MODULES), FOLL_WRITE);
    if (bytes_written < 0) {
        pr_err("<<klimem>> access_process_vm (write) failed\n");
    }
    mmput(reader_mm);
    put_task_struct(reader_task);
    kfree(mods);
    return;
}

static void GetProcesses(unsigned long proc_addr) {
    struct T_PROCESSES *procs = kmalloc(sizeof(struct T_PROCESSES), GFP_KERNEL);

    pid_t reader_pid = current->pid;
    struct task_struct *reader_task;
    struct mm_struct *reader_mm;
    // Locate the reader task
    reader_task = get_pid_task(find_get_pid(reader_pid), PIDTYPE_PID);
    if (!reader_task) {
        pr_err("<<klimem>> Could not find reader task for PID %d\n",
               reader_pid);
        kfree(procs);
        return;
    }

    reader_mm = get_task_mm(reader_task);
    if (!reader_mm) {
        pr_err("<<klimem>> Could not access reader map for target PID %d\n",
               reader_pid);
        put_task_struct(reader_task);
        kfree(procs);
        return;
    }
    procs->numProcesses = 0;
    struct task_struct *task;
    for_each_process(task) {
        if (procs->numProcesses == MAX_PROCESSES_READ) {
            break;
        }
        procs->processes[procs->numProcesses].pid = task->pid;
        strncpy(procs->processes[procs->numProcesses].name, task->comm,
                sizeof(procs->processes[procs->numProcesses].name));
        procs->numProcesses++;
    }

    int bytes_written = access_process_vm(
        reader_task, proc_addr, procs, sizeof(struct T_PROCESSES), FOLL_WRITE);
    if (bytes_written < 0) {
        pr_err("<<klimem>> access_process_vm (write) failed\n");
    }
    kfree(procs);
    mmput(reader_mm);
    put_task_struct(reader_task);
    return;
}

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

    // Write the data to the reader process
    bytes_written = access_process_vm(reader_task, args.buffer_address, buffer,
                                      bytes_read, FOLL_WRITE);
    if (bytes_written < 0) {
        pr_err("<<klimem>> access_process_vm (write) failed\n");
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
    unsigned long get_proc_buffer;
    struct T_MODULE_REQUEST mod_req;

    switch (cmd) {
    case IOCTL_RPM:
        if (copy_from_user(&rpm_val, (int __user *)arg, sizeof(struct T_RPM))) {
            return -EFAULT;
        }
        ReadProcessMemory(rpm_val);
        break;
    case IOCTL_GET_PROCESSES:
        if (copy_from_user(&get_proc_buffer, (int __user *)arg,
                           sizeof(unsigned long))) {
            return -EFAULT;
        }

        GetProcesses(get_proc_buffer);
        break;
    case IOCTL_GET_MODULES:
        if (copy_from_user(&mod_req, (int __user *)arg,
                           sizeof(struct T_MODULE_REQUEST))) {
            return -EFAULT;
        }
        ListModulesForProcess(mod_req);
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
