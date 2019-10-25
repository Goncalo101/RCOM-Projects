#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "connection.h"

static struct termios oldtio;

void terminal_setup(int fd) {
    struct termios newtio;

    int tc_attr_status = tcgetattr(fd, &oldtio);
    if (tc_attr_status == ERROR)
    {
        /* save current port settings */
        perror("tcgetattr error");
        exit(ERROR);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == ERROR)
    {
        perror("tcsetattr error");
        exit(ERROR);
    }

    printf("New termios structure set\n");
}

int llopen(int port, int mode) {
    char device[10];

    sprintf(device, "/dev/ttyS%d", port);

    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == ERROR) {
        perror(device);
        return fd;
    }

    terminal_setup(fd);

    return fd;
}

int llread(int fd, char *buffer) {
    int bytes_read = 0, accept = 0, res = 0;

    do {
        res = read(fd, &buffer[bytes_read], sizeof(char));
        if (res == ERROR) {
            perror("read failed");

            if (errno == EINTR) {
                printf("read timeout\n");
                return INTERRUPTED;
            }
            return ERROR;
        }
        
        // TODO: implement state machine 
    } while (!accept);

    return bytes_read;
}

int llwrite(int fd, char *buffer, int length) {
    int bytes_written = 0, res;

    for (; bytes_written < length; ++bytes_written) {
        res = write(fd, &buffer[bytes_written], sizeof(char));
        if (res == ERROR) {
            perror("write error");
            return ERROR;
        }

        printf("wrote hex: 0x%x ascii:%u\n", buffer[bytes_written], buffer[bytes_written]);
    }

    printf("wrote %d bytes\n", bytes_written);

    return bytes_written;
}