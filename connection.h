#ifndef CONNECTION_H
#define CONNECTION_H

#define BAUDRATE B38400

#define PACKET_HEAD_LEN 4

#define MAX_ALARM_COUNT 3
#define TIMEOUT 3

#define INTERRUPTED -2
#define ERROR -1

#define TYPE_A_PACKET_LENGTH 5
#define FRAME_I_LENGTH 6

typedef int (*sender_func)(int);

int llopen(int port, int mode);
int send_packet(int fd, char *fragment, int addr, int ctrl, int length);

#endif