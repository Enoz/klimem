
#include "linux/types.h"
#ifndef KERNEL_SPACE

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#endif /* ifndef KERNEL_SPACE */

#define MAX_PROCESSES_READ 10
#define DEVICE_NAME "/dev/klimem_dev"
#define MY_IOCTL_MAGIC 'k'
#define IOCTL_RPM _IOW(MY_IOCTL_MAGIC, 1, struct T_RPM)
#define IOCTL_GET_PROCESSES _IOR(MY_IOCTL_MAGIC, 2, struct T_PROCESSES)

struct T_RPM {
    pid_t target_pid;
    unsigned long target_address;
    size_t read_size;
    unsigned long buffer_address;
};

struct T_PROCESS {
    char name[16];
    pid_t pid;
};
struct T_PROCESSES {
    unsigned int numProcesses;
    struct T_PROCESS processes[MAX_PROCESSES_READ];
};
