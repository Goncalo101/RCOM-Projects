#ifndef CONNECTION_H
#define CONNECTION_H

#define BAUDRATE B38400

#define INTERRUPTED -2
#define ERROR -1

int llopen(int port, int mode);
int llread(int fd, char *buffer);

#endif