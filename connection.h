#ifndef CONNECTION_H
#define CONNECTION_H

#define BAUDRATE B38400
#define MAX_FRAGMENT_SIZE 65279

#define MAX_RETRIES 3
#define MAX_ALARM_COUNT 3
#define TIMEOUT 5
#define CTRL_POS 4

#define INTERRUPTED -2
#define ERROR -1

#include <termios.h>
#include "types.h"

typedef int (*sender_func)(int);

int llopen(int port, int mode, speed_t baudrate);
int send_packet(int fd, frame_t *frame);
int get_packet(int fd, frame_t *frame);
int llclose(int fd);

#endif