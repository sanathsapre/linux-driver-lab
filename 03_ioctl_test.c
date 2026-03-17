#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define DEVICE           "/dev/sanath_queue"
#define ETX_MAGIC        'e'
#define GET_QUEUE_SIZE   _IOR(ETX_MAGIC, 1, int32_t)
#define GET_MAX_CAPACITY _IOR(ETX_MAGIC, 2, int32_t)
#define CLEAR_QUEUE      _IO(ETX_MAGIC, 3)
#define RESET_DEVICE     _IO(ETX_MAGIC, 4)

int main()
{
    int fd;
    int32_t val;

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }
    printf("Opened %s successfully\n\n", DEVICE);

    /* GET_QUEUE_SIZE */
    if (ioctl(fd, GET_QUEUE_SIZE, &val) < 0)
        perror("GET_QUEUE_SIZE failed");
    else
        printf("GET_QUEUE_SIZE   : %d\n", val);

    /* GET_MAX_CAPACITY */
    if (ioctl(fd, GET_MAX_CAPACITY, &val) < 0)
        perror("GET_MAX_CAPACITY failed");
    else
        printf("GET_MAX_CAPACITY : %d\n", val);

    /* CLEAR_QUEUE */
    if (ioctl(fd, CLEAR_QUEUE) < 0)
        perror("CLEAR_QUEUE failed");
    else
        printf("CLEAR_QUEUE      : triggered successfully\n");

    /* verify queue is cleared */
    if (ioctl(fd, GET_QUEUE_SIZE, &val) < 0)
        perror("GET_QUEUE_SIZE after CLEAR_QUEUE failed");
    else
        printf("GET_QUEUE_SIZE after CLEAR_QUEUE : %d\n", val);

    /* RESET_DEVICE */
    if (ioctl(fd, RESET_DEVICE) < 0)
        perror("RESET_DEVICE failed");
    else
        printf("RESET_DEVICE     : triggered successfully\n");

    /* verify queue size after reset */
    if (ioctl(fd, GET_QUEUE_SIZE, &val) < 0)
        perror("GET_QUEUE_SIZE after RESET_DEVICE failed");
    else
        printf("GET_QUEUE_SIZE after RESET_DEVICE : %d\n", val);

    close(fd);
    printf("\nDone.\n");
    return 0;
}
