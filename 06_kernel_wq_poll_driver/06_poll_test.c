// 05_poll_blocking.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

#define DEVICE   "/dev/sanath_worker"
#define MEM_SIZE 128

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

    printf("Blocking poll — waiting up to 5 seconds for data...\n\n");

    pfd.fd     = fd;
    pfd.events = POLLIN;

    /* blocking poll with 5 second timeout */
    int ready = poll(&pfd, 1, 5000);

    if (ready < 0) {
        perror("poll failed");
        close(fd);
        return 1;
    }

    if (ready == 0) {
        printf("poll timed out after 5 seconds — no data available\n");
        close(fd);
        return 0;
    }

    if (pfd.revents & POLLIN) {
        memset(buf, 0, MEM_SIZE);
        ret = read(fd, buf, MEM_SIZE);
        if (ret > 0)
            printf("event: %s", buf);
        else if (ret == 0)
            printf("EOF\n");
        else
            perror("read failed");
    }

    close(fd);
    return 0;
}