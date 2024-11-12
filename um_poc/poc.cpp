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

    int testVal = 5738;
    int *readVal = (int *)malloc(sizeof(int));

    struct T_RPM to_send;
    to_send.target_pid = getpid();
    to_send.target_address = (unsigned long)&testVal;
    to_send.read_size = sizeof(int);
    to_send.buffer_address = (unsigned long)readVal;

    printf("Testing RPM::\n");
    printf("Value in testVal (0x%lx): %i\n", (unsigned long)&testVal, testVal);

    if (ioctl(fd, IOCTL_RPM, &to_send) < 0) {
        perror("IOCTL RPM failed");
        close(fd);
        return -1;
    }

    printf("Value in readVal (0x%lx): %i\n", (unsigned long)readVal, *readVal);

    printf("Testing GetProcesses::\n");
    struct T_PROCESSES *procs =
        (struct T_PROCESSES *)malloc(sizeof(struct T_PROCESSES));
    if (ioctl(fd, IOCTL_GET_PROCESSES, (unsigned long)(&procs)) < 0) {
        perror("IOCTL GET PROCS failed");
        close(fd);
        return -1;
    }

    printf("Found %i processes\n", procs->numProcesses);
    for (int i = 0; i < procs->numProcesses; i++) {
        printf("Proc %i: PID-%i Name: %s\n", i, procs->processes[i].pid,
               procs->processes[i].name);
    }

    close(fd);
    return 0;
}
