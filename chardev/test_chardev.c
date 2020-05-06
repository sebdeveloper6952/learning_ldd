#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    char buf[256];
    int fd;
    char *dev_name = "/dev/chardev0";

    fd = open(dev_name, O_RDWR);
    if (fd < 0)
    {
        printf("Error al leer device file.\n");
        return -1;
    }

    read(fd, buf, sizeof buf);
    printf("Se leyo de device file: %s\n", buf);

    close(fd);

    return 0;
}