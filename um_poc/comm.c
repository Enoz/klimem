#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEVICE_NAME "/dev/klimem_dev"
#define MY_IOCTL_MAGIC 'k'
#define IOCTL_RPM _IOW(MY_IOCTL_MAGIC, 1, struct T_RPM)

struct T_KL_RPM {
    int somenum;
    int someothernum;
};
