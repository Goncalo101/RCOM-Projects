#ifndef CONNECTION_H
#define CONNECTION_H

#define BAUDRATE B230400
#define MAX_FRAGMENT_SIZE 65279

#define MAX_ALARM_COUNT 3
#define TIMEOUT 5
#define CTRL_POS 4

#define INTERRUPTED -2
#define ERROR -1

#include "types.h"

typedef int (*sender_func)(int);

int llopen(int port, int mode);
int send_packet(int fd, frame_t *frame);
int get_packet(int fd, frame_t *frame);
int llclose(int fd);

#endif