#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define DEVICE      "/dev/sanath_worker"
#define MEM_SIZE    128

int main()
{
    int fd;
    char buf[MEM_SIZE];
    ssize_t ret;

    /* open in non-blocking mode */
    fd = open(DEVICE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }

    printf("Opened %s in non-blocking mode\n", DEVICE);

    ret = read(fd, buf, MEM_SIZE);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            printf("Buffer empty — non-blocking read returned EAGAIN as expected\n");
        else
            printf("read failed: %s\n", strerror(errno));
    } else if (ret == 0) {
        printf("Read returned 0 (EOF)\n");
    } else {
        printf("Read %zd bytes: %s\n", ret, buf);
    }

    close(fd);
    return 0;
}
