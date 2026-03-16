#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define DEVICE      "/dev/sanath_queue"
#define MEM_SIZE    128
#define TIMEOUT_MS  5000    /* 5 seconds */

int main()
{
    int fd;
    char buf[MEM_SIZE];
    ssize_t ret;
    struct pollfd pfd;

    fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }

    printf("Opened %s, polling for POLLIN...\n", DEVICE);

    pfd.fd      = fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;

    ret = poll(&pfd, 1, TIMEOUT_MS);
    if (ret < 0) {
        perror("poll failed");
        close(fd);
        return 1;
    } else if (ret == 0) {
        printf("poll timed out after %d ms — no data available\n", TIMEOUT_MS);
        close(fd);
        return 0;
    }

    /* poll returned, check what event fired */
    if (pfd.revents & POLLIN) {
        printf("POLLIN received — data available, reading...\n");
        ret = read(fd, buf, MEM_SIZE);
        if (ret < 0)
            perror("read failed");
        else if (ret == 0)
            printf("read returned 0 (EOF)\n");
        else
            printf("Read %zd bytes: %s\n", ret, buf);
    }

    if (pfd.revents & POLLERR)
        printf("POLLERR received\n");

    if (pfd.revents & POLLHUP)
        printf("POLLHUP received\n");

    close(fd);
    return 0;
}
