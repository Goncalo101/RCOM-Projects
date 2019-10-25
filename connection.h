#ifndef CONNECTION_H
#define CONNECTION_H

#define BAUDRATE B38400

#define INTERRUPTED -2
#define ERROR -1

#define TYPE_A_PACKET_LENGTH 5

typedef int (*sender_func)(int);

int llopen(int port, int mode);
int llread(int fd, char *buffer);
int llwrite(int fd, char *buffer, int length);

#endif