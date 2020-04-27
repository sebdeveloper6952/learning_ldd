#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(void)
{
    int fd = open("/dev/sebdev6992_kingston", O_RDWR);
    printf("fd: %d\n", fd);
    close(fd);

    return 0;
}