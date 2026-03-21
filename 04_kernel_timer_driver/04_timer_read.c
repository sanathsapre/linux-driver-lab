#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE    "/dev/sanath_timer"
#define MEM_SIZE  128

int main()
{
    int fd;
    char buf[MEM_SIZE + 1];    /* +1 for null terminator */
    ssize_t ret;
    int count = 0;

    fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }

    //while (1) {
        memset(buf, 0, sizeof(buf));    /* zero before each read */

        ret = read(fd, buf, MEM_SIZE);
        printf("ret = %ld", ret);

        if (ret > 0) {
            printf("event[%d]: %s", count++, buf);
        } else if (ret == 0) {
            //break;
            printf("%d\n", __LINE__);                      /* empty — EOF */
        } else {
            perror("read failed");
            //break;
        }
    //}

    if (count == 0)
        printf("Buffer empty\n");
    else
        printf("Total events read: %d\n", count);

    close(fd);
    return 0;
}