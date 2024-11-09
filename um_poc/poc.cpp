#include "../comm/comm.c"

#include <stdio.h>
#include <unistd.h>


int main() {

    printf("Current Pid: %i\n", getpid());

    int fd, val;

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    struct T_RPM toSend;
    toSend.target_pid = 1234;
    toSend.buffer_address = 0xDEADBEEF;
    toSend.read_size = 128;
    toSend.target_address = 0xDEADC0DE;
    if (ioctl(fd, IOCTL_RPM, &toSend) < 0) {
        perror("ioctl rpm set failed");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
