#include "../comm/comm.c"

#include <cstdlib>
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

    /*struct T_RPM toSend;*/
    /*toSend.target_pid = 1234;*/
    /*toSend.buffer_address = 0xDEADBEEF;*/
    /*toSend.read_size = 128;*/
    /*toSend.target_address = 0xDEADC0DE;*/
    /*if (ioctl(fd, IOCTL_RPM, &toSend) < 0) {*/
    /*    perror("ioctl rpm set failed");*/
    /*    close(fd);*/
    /*    return -1;*/
    /*}*/

    int testVal = 5738;
    int *readVal = (int *)malloc(sizeof(int));

    struct T_RPM to_send;
    to_send.target_pid = getpid();
    to_send.target_address = (unsigned long)&testVal;
    to_send.read_size = sizeof(int);
    to_send.buffer_address = (unsigned long)readVal;

    printf("Value in testVal (0x%lx): %i\n", (unsigned long)&testVal, testVal);

    if (ioctl(fd, IOCTL_RPM, &to_send) < 0) {
        perror("IOCTL RPM failed");
        close(fd);
        return -1;
    }

    printf("Value in readVal (0x%lx): %i\n", (unsigned long)readVal, *readVal);

    close(fd);
    return 0;
}
