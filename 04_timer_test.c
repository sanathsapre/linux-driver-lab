#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE       "/dev/sanath_timer"
#define ETX_MAGIC    'f'
#define START_TIMER  _IO(ETX_MAGIC, 1)
#define STOP_TIMER   _IO(ETX_MAGIC, 2)

int main()
{
    int fd;

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }

    printf("Starting timer...\n");
    if (ioctl(fd, START_TIMER) < 0) {
        perror("START_TIMER failed");
        close(fd);
        return 1;
    }
    printf("Timer started\n");

    sleep(5);

    printf("Stopping timer...\n");
    if (ioctl(fd, STOP_TIMER) < 0) {
        perror("STOP_TIMER failed");
        close(fd);
        return 1;
    }
    printf("Timer stopped\n");

    close(fd);
    return 0;
}