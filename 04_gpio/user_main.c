#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main ()
{
    int fd;
    char button_state;

    // Open file /dev/...
    fd = open("/dev/rpi_gpio", O_RDONLY); // che do read only
    if (fd < 0) {
        perror("Can not open file /dev/rpi_gpio \n");
        return -1;
    }

    printf("Read status button (gpio20) ....");
    printf("Press Ctrl+c eixt.\n");

    while (1) {
        // Read 1 byte status from driver
        if (read(fd, &button_state, 1) > 0) {
            printf("\r Status button press: %c", button_state);
            fflush(stdout); // day du lieu ra man hinh
        }

        // Dat lai vi tri con tro file ve dau (bat buoc vi ham read trong driver kiem tra offset)
        lseek(fd, 0, SEEK_SET);

        // Delay
        usleep(500000); // 500ms
    }

    close(fd);
    return 0;
}
