#include "../comm/comm.c"

#include <stdio.h>

struct T_RPM {
    int tVal1;
    int tVal2;
};

int main() {
    int fd, val;

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    struct T_RPM toSend;
    toSend.tVal1 = 5;
    toSend.tVal2 = 13;
    if (ioctl(fd, IOCTL_RPM, &toSend) < 0) {
        perror("ioctl rpm set failed");
        close(fd);
        return -1;
    }
    

    close(fd);
    return 0;
}
