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

int llopen(int port, int mode) {
    char device[10];
    struct termios newtio;

    sprintf(device, "/dev/ttyS%d", port);

    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == ERROR) {
        perror(device);
        return fd;
    }

    int tc_attr_status = tcgetattr(fd, &oldtio);
    if (tc_attr_status == ERROR){ 
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

    if (tcsetattr(fd, TCSANOW, &newtio) == ERROR) {
        perror("tcsetattr error");
        exit(ERROR);
    }

    printf("New termios structure set\n");

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
